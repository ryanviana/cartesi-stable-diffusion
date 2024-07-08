#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <map>
#include <array>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <fcntl.h>
#include <unistd.h>
#include "3rdparty/cpp-httplib/httplib.h"
#include "3rdparty/picojson/picojson.h"

// Global path variables
// const std::string MODEL_PATH = "./sd-pixel-model.ckpt";
// const std::string OUTPUT_PATH = "./output.png";
// const std::string BASE64_COMMAND = "base64 -i ";
// const std::string SD_COMMAND = "./sd-mac";

const std::string MODEL_PATH = "/opt/cartesi/dapp/sd-pixel-model.ckpt";
const std::string OUTPUT_PATH = "/opt/cartesi/dapp/images/output.png";
const std::string BASE64_COMMAND = "base64 -i ";
const std::string SD_COMMAND = "/opt/cartesi/dapp/sd";

// Function to decode hex string to ASCII
std::string hex_to_string(const std::string &input)
{
    const char *const hex_chars = "0123456789abcdef";
    std::string output;
    output.reserve(input.length() / 2);

    for (std::string::size_type i = 0; i < input.length(); i += 2)
    {
        char high = input[i];
        char low = input[i + 1];
        output.push_back((char)((std::find(hex_chars, hex_chars + 16, high) - hex_chars) * 16 + (std::find(hex_chars, hex_chars + 16, low) - hex_chars)));
    }
    return output;
}

// Function to run a command and capture its output
std::string run_command(const std::string &command)
{
    std::array<char, 128> buffer;
    std::string result;
    FILE *pipe = popen((command + " 2>&1").c_str(), "r");
    if (!pipe)
    {
        std::cerr << "Failed to run command." << std::endl;
        return "";
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
    {
        std::string line(buffer.data());
        std::cout << line; // Print each line as it arrives
        result += line;
    }
    pclose(pipe);
    return result;
}

// Function to run stable diffusion and generate an image
// Function to run stable diffusion and generate an image
std::string run_stable_diffusion(const std::string &model_path, const picojson::object &params, const std::string &output_path)
{
    // Safe access to parameters from the JSON object
    std::string prompt = params.at("prompt").get<std::string>();
    int steps = static_cast<int>(params.at("steps").get<double>());
    float cfg_scale = static_cast<float>(params.at("cfg_scale").get<double>());
    int height = static_cast<int>(params.at("height").get<double>());
    int width = static_cast<int>(params.at("width").get<double>());
    std::string sampling_method = params.at("sampling_method").get<std::string>();

    // Building the command with extracted parameters
    std::ostringstream command;
    command << SD_COMMAND << " -m " << model_path
            << " -p \"" << prompt << "\""
            << " --type f16"
            << " --steps " << steps
            << " -H " << height
            << " -W " << width
            << " --sampling-method " << sampling_method
            << " --cfg-scale " << cfg_scale
            << " -o " << output_path;

    std::cout << "Executing command: " << command.str() << std::endl;

    std::string result = run_command(command.str());
    std::cout << "Command output: " << result << std::endl;

    if (result.find("error") == std::string::npos)
    {
        std::cout << "Image generated successfully at " + output_path << std::endl;
        return "accept";
    }
    else
    {
        std::cerr << "Failed to generate image. Command output: " << result << std::endl;
        return "reject";
    }
}

// Function to encode a file in base64
std::string base64_encode(const std::string &file_path)
{
    std::string command = BASE64_COMMAND + file_path;
    std::string encoded_content = run_command(command);
    return encoded_content;
}

// Function to send a report to the rollup server
void send_report(httplib::Client &cli, const std::string &report_content)
{
    std::ostringstream hex_report;
    hex_report << "0x";
    for (unsigned char c : report_content)
    {
        hex_report << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }

    picojson::object report;
    report["payload"] = picojson::value(hex_report.str());

    std::string report_json = picojson::value(report).serialize();
    std::cout << "Sending report: " << report_json << std::endl;

    auto res = cli.Post("/report", report_json, "application/json");
    if (res)
    {
        if (res->status == 200)
        {
            std::cout << "Report sent successfully." << std::endl;
        }
        else
        {
            std::cerr << "Failed to send report. Status code: " << res->status << std::endl;
            std::cerr << "Response body: " << res->body << std::endl;
        }
    }
    else
    {
        std::cerr << "Failed to send report. HTTP error: " << res.error() << std::endl;
    }
}

std::string handle_advance(httplib::Client &cli, picojson::value data)
{
    std::cout << "Received advance request data: " << data << std::endl;

    // Extract and decode payload from the input data
    std::string payload_hex = data.get("payload").get<std::string>();
    std::string payload_json = hex_to_string(payload_hex.substr(2)); // Remove '0x' prefix and decode hex

    picojson::value payload;
    std::string err = picojson::parse(payload, payload_json);
    if (!err.empty())
    {
        std::cerr << "Error parsing payload JSON: " << err << std::endl;
        return "reject";
    }

    if (!payload.is<picojson::object>())
    {
        std::cerr << "Error: Payload is not a JSON object." << std::endl;
        return "reject";
    }

    picojson::object params = payload.get<picojson::object>();

    // Check if model path exists
    if (access(MODEL_PATH.c_str(), F_OK) == -1)
    {
        std::cerr << "Model path does not exist: " << MODEL_PATH << std::endl;
        return "reject";
    }

    // Run stable diffusion
    std::string status = run_stable_diffusion(MODEL_PATH, params, OUTPUT_PATH);

    // If image generation is successful, encode the image and send a report
    if (status == "accept")
    {
        // Ensure the file exists before encoding
        if (access(OUTPUT_PATH.c_str(), F_OK) != -1)
        {
            std::string encoded_image = base64_encode(OUTPUT_PATH);
            send_report(cli, encoded_image);
        }
        else
        {
            std::cerr << "Error: Generated image file not found at " << OUTPUT_PATH << std::endl;
            status = "reject";
        }
    }

    return status;
}

std::string handle_inspect(httplib::Client &cli, picojson::value data)
{
    std::cout << "Received inspect request data: " << data << std::endl;
    return "accept";
}

int main(int argc, char **argv)
{
    const char *rollup_server_url = getenv("ROLLUP_HTTP_SERVER_URL");
    if (!rollup_server_url)
    {
        std::cerr << "Error: ROLLUP_HTTP_SERVER_URL environment variable is not set." << std::endl;
        return 1;
    }

    std::map<std::string, decltype(&handle_advance)> handlers = {
        {"advance_state", &handle_advance},
        {"inspect_state", &handle_inspect},
    };

    httplib::Client cli(rollup_server_url);
    cli.set_read_timeout(20, 0);
    std::string status("accept");

    while (true)
    {
        std::cout << "Sending finish" << std::endl;
        auto finish = std::string("{\"status\":\"") + status + std::string("\"}");
        auto r = cli.Post("/finish", finish, "application/json");
        if (!r)
        {
            std::cerr << "Error: Failed to send /finish request." << std::endl;
            continue;
        }

        std::cout << "Received finish status: " << r->status << std::endl;
        if (r->status == 202)
        {
            std::cout << "No pending rollup request, trying again" << std::endl;
        }
        else
        {
            picojson::value rollup_request;
            std::string err = picojson::parse(rollup_request, r->body);
            if (!err.empty())
            {
                std::cerr << "Error parsing rollup request: " << err << std::endl;
                status = "reject";
                continue;
            }

            std::cout << "Parsed rollup request: " << rollup_request << std::endl;

            auto request_type = rollup_request.get("request_type").get<std::string>();
            auto handler_it = handlers.find(request_type);
            if (handler_it == handlers.end())
            {
                std::cerr << "Unknown request type: " << request_type << std::endl;
                status = "reject";
                continue;
            }

            auto data = rollup_request.get("data");
            std::cout << "Handler found for request type: " << request_type << std::endl;
            status = handler_it->second(cli, data);
        }
    }
    return 0;
}
