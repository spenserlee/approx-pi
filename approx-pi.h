#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <limits>
#include <string>
#include <thread>

#include "utils.h"

#define ACTUAL_PI "3.14159265358979323846"
#define SEMAPHORE_NAME "/approx_pi_sem"

// to monitor semaphores
// watch -n1 ls -al /dev/shm/sem.*|more

/*
RESOURCES USED
POSIX semaphore resource
http://www.frascati.enea.it/documentation/tru6450/ARH9TATE/SMCHPXXX.HTM
NOTE posix semaphores cannot be path names
A named semaphore is identified by a name of the form /somename; that is,
a null-terminated string of up to NAME_MAX-4 (i.e., 251) characters
consisting of an initial slash, followed by one or more characters,
none of which are slashes.
source: https://linux.die.net/man/7/sem_overview

argp resource
https://www.gnu.org/software/libc/manual/html_node/Argp.html#Argp
*/

/*
It is understood that using the Taylor series to approximate pi is very slow,
this program is simply to compare the speed between processes and threads in a long running task.

https://www.math.hmc.edu/funfacts/ffiles/30001.1-3.shtml
http://www.geom.uiuc.edu/~huberty/math5337/groupe/expresspi.html?
*/

    // pi       = 4 * ( 1 - 1/3 + 1/5 - 1/7 + 1/9 - 1/11 ... )
    // let y    = - 1/3 + 1/5 - 1/7 + 1/9 - 1/11 + 1/13 - 1/15 + 1/17 ...)
    // let z    = ( 1 + y )
    // pi       = 4 * z
static long double *global_result;

void approx_pi_t(
    unsigned long long start,
    unsigned long long num_iters,
    unsigned long long total_iters,
    bool output,
    int process,
    int thread
    );

void approx_pi(
    unsigned long long start,
    unsigned long long num_iters,
    unsigned long long total_iters,
    bool output,
    int process
    );

