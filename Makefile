NASM := nasm

STRIDE := 64
ASM_FLAGS := -DFIRSTO=0


override CXXFLAGS += -fmessage-length=0 -Wall -Wextra -std=gnu++11 -g -O1 -DSTRIDE=${STRIDE}

all: weirdo-main

weirdo-main.o: weirdo-main.cpp cycle-timer.hpp Makefile

weirdo-main: weirdo-main.o weirdo.o weirdo-cpp.o huge-alloc.o page-info.o
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.asm Makefile
	$(NASM) $(ASM_FLAGS) -DSTRIDE=${STRIDE} -f elf64 -o $@ $<

clean:
	rm -f weirdo-main
	rm -f *.o
