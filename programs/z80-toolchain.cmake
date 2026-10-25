# CMake toolchain file for the z80-elf compiler in ../install.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR z80)

get_filename_component(Z80_INSTALL "${CMAKE_CURRENT_LIST_DIR}/../install" ABSOLUTE)
set(CMAKE_C_COMPILER   ${Z80_INSTALL}/bin/z80-elf-gcc)
set(CMAKE_CXX_COMPILER ${Z80_INSTALL}/bin/z80-elf-g++)
set(CMAKE_OBJCOPY      ${Z80_INSTALL}/bin/z80-elf-objcopy CACHE FILEPATH "")

# The compiler cannot link a test program without a linker script.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
