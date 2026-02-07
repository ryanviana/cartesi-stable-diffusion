# Cartesi Stable Diffusion

![C++](https://img.shields.io/badge/C++-11-blue?logo=cplusplus)
![Cartesi](https://img.shields.io/badge/Cartesi_Rollups-0.6.2-purple)
![License](https://img.shields.io/badge/License-MIT-green)

On-chain AI image generation powered by Stable Diffusion, running inside a [Cartesi](https://cartesi.io/) Rollups dApp. Users submit text prompts as blockchain transactions, and the dApp generates images deterministically inside the Cartesi Machine — a RISC-V virtual machine that enables verifiable computation on-chain.

## How It Works

```
Blockchain (L1/L2)
    │
    ▼
Cartesi Rollup Server
    │  advance_state
    ▼
dapp (C++ binary)
    │── Decodes hex-encoded JSON payload
    │── Extracts parameters (prompt, steps, cfg_scale, etc.)
    │── Invokes Stable Diffusion binary (RISC-V)
    │── Base64-encodes the generated PNG
    └── Sends image back as a rollup report
```

1. A user sends a transaction with a JSON payload containing image generation parameters
2. The Cartesi Machine receives the input and dispatches it to the C++ dApp
3. The dApp invokes the Stable Diffusion binary with the specified parameters
4. The generated image is base64-encoded and returned as a rollup report
5. Because it runs in the deterministic Cartesi VM, the output is cryptographically verifiable

## Tech Stack

- **C++11** — Core dApp logic (`dapp.cpp`)
- **Stable Diffusion** — AI image generation (pre-compiled for RISC-V)
- **Cartesi Rollups SDK 0.6.2** — Blockchain rollup infrastructure
- **cpp-httplib** — HTTP client for rollup server communication
- **picojson** — JSON parsing
- **Docker** — Multi-stage build targeting `riscv64/ubuntu:22.04`

## Features

- **On-chain AI image generation** — Submit prompts as blockchain transactions
- **Configurable parameters** — Control prompt, steps, CFG scale, dimensions, and sampling method
- **Verifiable computation** — Deterministic execution inside the Cartesi RISC-V VM
- **Pixel art model** — Uses a specialized Stable Diffusion checkpoint optimized for pixel art

## Getting Started

### Prerequisites

- [Docker](https://www.docker.com/) with RISC-V support
- [Cartesi CLI](https://docs.cartesi.io/)

### Setup

1. **Clone the repository:**
   ```bash
   git clone https://github.com/ryanviana/cartesi-stable-diffusion.git
   cd cartesi-stable-diffusion
   ```

2. **Download the model checkpoint:**

   Download `sd-pixel-model.ckpt` from [Google Drive](https://drive.google.com/file/d/1upykEtLpJkNY33RMcv8W1mTUN8z3WkAk/view?usp=drive_link) and place it in the project root.

3. **Build and run:**
   ```bash
   cartesi build
   cartesi run
   ```

### Sending a Request

Submit a hex-encoded JSON payload as a transaction:

```json
{
  "prompt": "a pixel art castle",
  "steps": 20,
  "cfg_scale": 7.0,
  "height": 256,
  "width": 256,
  "sampling_method": "euler_a"
}
```

| Parameter | Description |
|---|---|
| `prompt` | Text description of the image to generate |
| `steps` | Number of inference steps |
| `cfg_scale` | Classifier-free guidance scale |
| `height` | Image height in pixels |
| `width` | Image width in pixels |
| `sampling_method` | Sampling algorithm (e.g., `euler_a`) |

## Video Tutorial

[![Video Tutorial](https://img.shields.io/badge/YouTube-Tutorial-red?logo=youtube)](https://youtu.be/XJKx2pitSc0)

## Related Repositories

- [stable-diffusion-on-riscv](https://github.com/ryanviana/stable-diffusion-on-riscv) — Stable Diffusion compiled for RISC-V
- [ctsi-sd-cpp](https://github.com/ryanviana/ctsi-sd-cpp) — C++ implementation of SD for Cartesi
- [ctsi-rust-sd](https://github.com/ryanviana/ctsi-rust-sd) — Rust implementation of SD for Cartesi
- [cartesi-wheels](https://github.com/ryanviana/cartesi-wheels) — Pre-built Python wheels for Cartesi RISC-V

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
