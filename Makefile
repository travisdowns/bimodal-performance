NASM := nasm

STRIDE := 64
ASM_FLAGS := -DFIRSTO=64 -DSECONDO=0

override CXXFLAGS += -fmessage-length=0 -Wall -Wextra -std=gnu++11 -g -O2 -DSTRIDE=${STRIDE} -march=haswell
LDFLAGS = -use-ld=gold

HEADERS := $(wildcard *.h *.hpp)

all: weirdo-main

weirdo-main: weirdo-main.o weirdo.o weirdo-cpp.o huge-alloc.o page-info.o
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $< 

%.o: %.asm Makefile
	$(NASM) $(ASM_FLAGS) -DSTRIDE=${STRIDE} -f elf64 -o $@ $<

clean:
	rm -f weirdo-main
	rm -f *.o
