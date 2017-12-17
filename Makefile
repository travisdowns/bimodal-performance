NASM := yasm

CXXFLAGS += -fmessage-length=0 -Wall -Wextra -std=gnu++11 -g -O1 

all: weirdo-main

weirdo-main.o: weirdo-main.cpp cycle-timer.hpp

weirdo-main: weirdo-main.o weirdo.o weirdo-cpp.o
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^
	
weirdo-main.o: weirdo-main.cpp cycle-timer.hpp

%.o: %.asm
	$(NASM) -f elf64 -o $@ $<

clean:
	rm -f weirdo-main
	rm -f *.o 


