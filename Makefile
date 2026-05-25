CC := wcc
LD := wlink
CFLAGS := -q -2 -Oaxrht
LDFLAGS :=
export WATCOM ?= $(realpath $(dir $(shell command -v $(CC)))..)
export INCLUDE ?= $(WATCOM)/h
export LIB ?= $(WATCOM)/lib286

SRCS := wolfmaus.c
OBJS := $(patsubst %.c, %.o, $(SRCS))

.PHONY: all clean

all: wolfmaus.exe

%.o: %.c
	$(CC) $(CFLAGS) $< -fo=$@

wolfmaus.lnk: $(OBJS)
	{ printf 'option quiet\nsystem dos\n'; printf 'file %s\n' $^; printf 'name wolfmaus\n'; } > $@

wolfmaus.exe: wolfmaus.lnk
	$(LD) $(LDFLAGS) @$<

clean:
	rm -f $(OBJS) wolfmaus.lnk wolfmaus.exe
