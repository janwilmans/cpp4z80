## Z80 board support.

## MSX-DOS / CP/M 2.2 .com programs (-specs=msxdos.specs).
multilibtool_DATA += \
	%D%/crt0-msxdos.o \
	%D%/msxdos.ld \
	%D%/msxdos.specs
libobjs_a_SOURCES += \
	%D%/crt0-msxdos.S
multilibtool_LIBRARIES += %D%/libmsxdos.a
%C%_libmsxdos_a_SOURCES = \
	%D%/msxdos-bdos.S \
	%D%/msxdos-args.c \
	%D%/msxdos-exit.c \
	%D%/msxdos-misc.c \
	%D%/msxdos-read.c \
	%D%/msxdos-sbrk.c \
	%D%/msxdos-write.c
