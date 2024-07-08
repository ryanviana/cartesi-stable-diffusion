# syntax=docker.io/docker/dockerfile:1
FROM --platform=linux/riscv64 riscv64/ubuntu:22.04 AS builder

ARG DEBIAN_FRONTEND=noninteractive
RUN <<EOF
set -e
apt-get update
apt-get install -y --no-install-recommends \
  autoconf \
  automake \
  build-essential \
  ca-certificates \
  curl \
  libtool \
  wget
rm -rf /var/lib/apt/lists/*
EOF

WORKDIR /opt/cartesi/dapp
COPY . .
RUN make

FROM --platform=linux/riscv64 riscv64/ubuntu:22.04

ARG MACHINE_EMULATOR_TOOLS_VERSION=0.14.1
ADD https://github.com/cartesi/machine-emulator-tools/releases/download/v${MACHINE_EMULATOR_TOOLS_VERSION}/machine-emulator-tools-v${MACHINE_EMULATOR_TOOLS_VERSION}.deb /
RUN dpkg -i /machine-emulator-tools-v${MACHINE_EMULATOR_TOOLS_VERSION}.deb \
  && rm /machine-emulator-tools-v${MACHINE_EMULATOR_TOOLS_VERSION}.deb

LABEL io.cartesi.rollups.sdk_version=0.6.2
LABEL io.cartesi.rollups.ram_size=3000Mi

ARG DEBIAN_FRONTEND=noninteractive
RUN <<EOF
set -e
apt-get update
apt-get install -y --no-install-recommends \
  busybox-static=1:1.30.1-7ubuntu3
rm -rf /var/lib/apt/lists/* /var/log/* /var/cache/*
useradd --create-home --user-group dapp
EOF

ENV PATH="/opt/cartesi/bin:${PATH}"

WORKDIR /opt/cartesi/dapp
COPY --from=builder /opt/cartesi/dapp/dapp .
COPY sd-risc /opt/cartesi/dapp/sd
COPY sd-pixel-model.ckpt /opt/cartesi/dapp/sd-pixel-model.ckpt
RUN chmod +x /opt/cartesi/dapp/sd
RUN mkdir -p /opt/cartesi/dapp/images
COPY images/* /opt/cartesi/dapp/images/
RUN chown -R dapp:dapp /opt/cartesi/dapp

ENV ROLLUP_HTTP_SERVER_URL="http://127.0.0.1:5004"

USER dapp

ENTRYPOINT ["rollup-init"]
CMD ["/opt/cartesi/dapp/dapp"]
