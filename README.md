# bimodal-performance

Test code and hardness weird bimodal performance on Intel Skylake CPU [discussed on stackoverflow](https://stackoverflow.com/questions/47851120/unexpectedly-poor-and-bimodal-performance-for-simple-store-loop-on-intel-skylake).

## Building

### Unix-like

    make
    
## Running

You can run either the C++ or asm versions of the loop. They should have the same beahvior (but if they don't, you should check the generated assembly for the C++ version).

Run the C++ loop:

    ./weirdo-main c++

Run the asm loop:

    ./weirdo-main asm
    
If you don't see the weird performance on the first run, you can run it repeatedly like (if you're using bash):

    while [ true ]; do sudo taskset -c 2 ./weirdo-main asm; done
