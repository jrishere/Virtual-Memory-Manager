README.txt
----------

Title: Virtual Memory Manager

Description:
This project simulates various page replacement algorithms to demonstrate how operating systems manage virtual memory. It includes the implementation of strategies such as LIFO (Last In, First Out), MRU (Most Recently Used), LFU (Least Frequently Used), and OPT (Optimal Page Replacement) with lookahead capabilities.

Compiling the Code:
1. Open your command terminal and navigate to the directory containing the project files.
2. Compile the code using the following command:
g++ -o VirtualMemoryManager main.cpp

Running the Simulation:
1. Run the compiled program:
./VirtualMemoryManager

2. Ensure that an input file named `input.txt` is in the same directory as the executable. This file should contain the simulation configurations as specified in the source code documentation.

Input File Format:
The input file `input.txt` should follow the format specified in the source code comments, including parameters like total number of page frames, page size, number of page frames per process, and the page access sequences for each process.
