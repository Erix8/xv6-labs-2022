# Dockerfile
FROM ubuntu:20.04

# 禁用交互式配置
ENV DEBIAN_FRONTEND=noninteractive

# 安装编译和调试依赖
RUN apt update && \
    apt install -y build-essential git python3 gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu curl zsh && \
    rm -rf /var/lib/apt/lists/*

# 设置工作目录
WORKDIR /root/xv6-labs-2022

# 启动时进入 zsh
CMD ["zsh"]