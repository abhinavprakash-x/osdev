# Toolchain Setup (i686-elf-gcc)

To build this OS, you cannot use your host system's default GCC compiler, as it will try to compile code for your current operating system (e.g., Linux). Instead, we need a **cross-compiler** configured for the `i686-elf` target (a generic 32-bit x86 executable format).

## 1. Install Prerequisites
You will need standard build tools and QEMU for emulation. Run this on your host machine:

```bash
sudo apt update
sudo apt install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo wget git nasm qemu-system-x86
```

## 2. Download Sources
Download the GNU `binutils` and `i686-elf-gcc` Source Code

```bash
wget https://ftp.gnu.org/gnu/binutils/binutils-2.41.tar.gz
wget https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.gz
```

## 3. Extract Sources
```bash
tar -xvf binutils-2.41.tar.gz
tar -xvf gcc-13.2.0.tar.gz
mkdir build-binutils
mkdir build-gcc
```

## 4. Build `binutils`
```bash
cd ~/osdev/build-binutils
../binutils-2.41/configure \
  --target=i686-elf --prefix="$HOME/osdev/cross" \
  --disable-nls --disable-werror
make -j$(nproc)
make install
```

## 5. Build `GCC`
```bash
cd ~/osdev/build-gcc
../gcc-13.2.0/configure \
  --target=i686-elf --prefix="$HOME/osdev/cross" \
  --disable-nls --enable-languages=c --without-headers
make all-gcc -j$(nproc)
make all-target-libgcc -j$(nproc)

make install-gcc
make install-target-libgcc
```

## 6. Update Paths
To ensure your terminal can find the new cross-compiler, add it to your `.bashrc` file:

```bash
echo 'export PATH="$HOME/osdev/cross/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

Now You can build and run the OS.