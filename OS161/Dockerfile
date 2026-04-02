FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    bison \
    flex \
    libncurses-dev \
    gawk \
    texinfo \
    python3 \
    python3-pip \
    wget \
    curl \
    ca-certificates \
    vim \
    tmux \
    gdb \
    && rm -rf /var/lib/apt/lists/*

# Create working directory
WORKDIR /root

# Copy setup script
COPY setup.sh /root/setup.sh
RUN chmod +x /root/setup.sh

# Run setup
RUN /root/setup.sh

# Set working dir to os161
WORKDIR /root/os161/root

# Default shell
CMD ["/bin/bash"]