NASM := yasm

CXXFLAGS += -fmessage-length=0 -Wall -Wextra -std=gnu++11 -g -Og 

all: weirdo-main

weirdo-main: weirdo-main.o weirdo.o
	g++ -o $@ $^

%.o: %.asm
	$(NASM) -f elf64 -o $@ $<

clean:
	rm -f weirdo-main
	rm -f *.o 


