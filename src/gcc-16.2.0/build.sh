rm -rf ~/project/cpp4z80/src/build-z80
mkdir -p ~/project/cpp4z80/src/build-z80
cd ~/project/cpp4z80/src/build-z80

../gcc-16.2.0/configure \
    --target=z80-elf \
    --enable-languages=c \
    --disable-nls \
    --disable-werror

make -j$(nproc) all-gcc

