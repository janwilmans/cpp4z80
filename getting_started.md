
# gcc developer setup

# setup a project

- jan@vortex:~/project/cpp4z80$ mkdir -p src build install work
- $ cd ~/project/cpp4z80/src
- $ wget https://ftp.gnu.org/gnu/gcc/gcc-16.2.0/gcc-16.2.0.tar.xz
- $ tar -xf gcc-16.2.0.tar.xz

# add binutils sources

- cd ~/project/cpp4z80/src
- wget https://ftp.gnu.org/gnu/binutils/binutils-2.45.tar.xz
- tar -xf binutils-2.45.tar.xz