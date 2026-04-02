#!/bin/bash
set -e

echo "=== OS/161 Setup Started ==="

# Create base directory
mkdir -p /root/os161
cd /root/os161

# Clone repositories
git clone https://github.com/ops-class/os161.git
git clone https://github.com/ops-class/sys161.git

# =============================
# Build toolchain
# =============================
echo "=== Building Toolchain ==="

cd os161
./configure --ostree=/root/os161/root --toolprefix=mips-harvard-os161-
make
make install

# Add toolchain to PATH
echo 'export PATH=/root/os161/root/tools/bin:$PATH' >> /root/.bashrc
export PATH=/root/os161/root/tools/bin:$PATH

# =============================
# Build System/161
# =============================
echo "=== Building System/161 ==="

cd /root/os161/sys161
./configure
make
make install

# =============================
# Build OS/161 Kernel
# =============================
echo "=== Building OS/161 Kernel ==="

cd /root/os161/os161/kern/conf
./config ASST0

cd ../compile/ASST0
bmake depend
bmake
bmake install

echo "=== Setup Complete ==="