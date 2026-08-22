rm -rf ~/project/cpp4z80/src/build-z80
mkdir -p ~/project/cpp4z80/src/build-z80
cd ~/project/cpp4z80/src/build-z80

../gcc-16.2.0/configure \
    --prefix="$HOME/project/cpp4z80/install" \
    --target=z80-elf \
    --enable-languages=c \
    --disable-nls \
    --disable-werror \
    CFLAGS="-O0 -g3" \
    CXXFLAGS="-O0 -g3"

# build only gcc itself first
make -j$(nproc) all-gcc

#make -j$(nproc) all
