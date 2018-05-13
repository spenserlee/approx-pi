# approx-pi
This project was an exercise to compare the performance between processes and threads with some busy-work operation and file I/O. The chosen busy-work operation was to calculate an approximation of Pi using the [Taylor series](https://www.math.hmc.edu/funfacts/ffiles/30001.1-3.shtml). The number of iterations within the Taylor series can be divided between workers to speed up the calculation.

My (rather unscientific and not controlled) testing on Linux did not find much of a performance difference between threads and processes for this particular task.

Approx Pi was is written in C++ 11, uses std::threads for thread workers, fork() for processes, and [argp](https://www.gnu.org/software/libc/manual/html_node/Argp.html) for argument parsing. 

![approx-pi](https://github.com/spenserlee/approx-pi/blob/master/documentation/pics/approx-pi.PNG)

## Directory Contents
	  ├── documentation
	  │   ├── pics/
	  │   ├── Design.pdf
	  │   ├── Report.pdf
	  │   └── Testing.pdf
	  ├── src
	  │   ├── approx-pi.cpp
	  │   ├── approx-pi.h
	  │   ├── makefile
	  │   └── utils.h
	  ├── .gitignore
	  └── README.md

## Running
Compile the program by navigating to the src folder and running `make`.

	Usage: approx-pi [OPTION...] NUM_ITERATIONS NUM_PROCESSES
	  -o, --output               disable worker I/O operations
	  -t, --threads=AMOUNT       number of worker threads PER process
	  -?, --help                 Give this help list
	      --usage                Give a short usage message
	  -V, --version              Print program version

**I recommend running the program with the `-o` flag** as this disables the I/O component for the workers which may generate a large amount (GBs) of data if  running with many iterations.

For example:

    ./approx-pi -o 1000 4
will run 1000 total iterations across 4 processes with I/O disabled.

    ./approx-pi -o 1000 1 -t 4
will run 1000 total iterations across **1 process which has 4 threads of execution** with I/O disabled.


## Todo
 - Arbitrary length precision (probably would take too long or a lot of cores anyways...)
 - Simplify giant comments
 - Compare with an [OpenMP](https://bisqwit.iki.fi/story/howto/openmp/#IntroductionToOpenmpInC) implementation
