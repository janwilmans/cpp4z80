cmake_minimum_required(VERSION 3.16)

# Sample MSX-DOS / CP/M .com programs, built as small as possible.
#
#   cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=z80-toolchain.cmake
#   cmake --build build
#   cmake --build build --target sizes     # table of .com sizes
#   cmake --build build --target run-hello # run one on the PC
project(msxdos_programs C CXX)
set(CMAKE_CXX_STANDARD 20)

set(RUN_COM ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/run-com.py)

# Size-oriented flags shared by all programs.
set(SIZE_FLAGS -Os -fno-exceptions -fomit-frame-pointer
    -fno-asynchronous-unwind-tables -fno-unwind-tables
    -ffunction-sections -fdata-sections -flto)
set(CXX_SIZE_FLAGS -fno-rtti -fno-threadsafe-statics)

set(COM_TARGETS "")

# add_com (NAME MODE SOURCE...)
#   MODE tiny   - freestanding: no C library, no start-up code (tiny/tiny.ld)
#   MODE newlib - normal program with -specs=msxdos.specs
function(add_com name mode)
  set(elf ${name}.elf)
  add_executable(${elf} ${ARGN})
  target_compile_options(${elf} PRIVATE ${SIZE_FLAGS}
    $<$<COMPILE_LANGUAGE:CXX>:${CXX_SIZE_FLAGS}>)
  if(mode STREQUAL "tiny")
    target_include_directories(${elf} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tiny
      ${CMAKE_CURRENT_SOURCE_DIR}/tiny-cpp)
    target_compile_options(${elf} PRIVATE -ffreestanding)
    target_link_libraries(${elf} PRIVATE gcc)
    target_link_options(${elf} PRIVATE ${SIZE_FLAGS} -nostdlib -nostartfiles
      -T ${CMAKE_CURRENT_SOURCE_DIR}/tiny/tiny.ld -Wl,--gc-sections)
  else()
    target_link_options(${elf} PRIVATE ${SIZE_FLAGS} -specs=msxdos.specs)
  endif()
  add_custom_command(TARGET ${elf} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${elf}> ${name}.com
    BYPRODUCTS ${name}.com)
  add_custom_target(${name} ALL DEPENDS ${elf})
  add_custom_target(run-${name}
    COMMAND python3 ${RUN_COM} ${CMAKE_CURRENT_BINARY_DIR}/${name}.com
    DEPENDS ${name} USES_TERMINAL)
  set(COM_TARGETS ${COM_TARGETS} ${name} PARENT_SCOPE)
  set(COM_LIST "${COM_LIST};${mode}:${name}" PARENT_SCOPE)
endfunction()

# Freestanding programs.
add_com(nop       tiny tiny/nop.c)
add_com(hello     tiny tiny/hello.c)
add_com(echo      tiny tiny/echo.c)
add_com(upper     tiny tiny/upper.c)
add_com(fib       tiny tiny/fib.c)
add_com(countdown tiny tiny/countdown.cpp)

# C++ on top of tiny.h: constexpr builds the '$'-terminated string.
add_com(hello_cpp tiny tiny-cpp/hello.cpp)
add_com(hello_udl tiny tiny-cpp/hello_udl.cpp)

# The same hello world through the C library, for comparison.
add_com(hello_write  newlib newlib/hello_write.cpp)
add_com(hello_puts   newlib newlib/hello_puts.cpp)
add_com(hello_printf newlib newlib/hello_printf.cpp)

# Print the size of every .com file.
set(SIZES_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/sizes.cmake)
file(WRITE ${SIZES_SCRIPT} "
set(total 0)
message(\"\")
message(\"program               bytes\")
message(\"----------------------------\")
foreach(item ${COM_LIST})
  if(item)
    string(REGEX REPLACE \"^[a-z]+:\" \"\" n \${item})
    file(SIZE ${CMAKE_CURRENT_BINARY_DIR}/\${n}.com s)
    string(LENGTH \${n} len)
    math(EXPR pad \"22 - \${len}\")
    string(REPEAT \" \" \${pad} sp)
    message(\"\${n}.com\${sp}\${s}\")
  endif()
endforeach()
")
add_custom_target(sizes COMMAND ${CMAKE_COMMAND} -P ${SIZES_SCRIPT}
  DEPENDS ${COM_TARGETS} USES_TERMINAL)
