#include <stdio.h>
#include <unistd.h>
#include <math.h>
// #include <sys/ipc.h>
// #include <sys/sem.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <limits>

#define PATH_MAX 255
#define NUM_WORKERS 8
#define ITERATIONS 1000000000L
#define ACTUAL_PI "3.14159265358979323846"

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


void serial_test()
{

    long double seriesResult = 0.0;
    long double denominator = 3.0;
    long double approx_pi = 0.0;
    char sign = -1;

    for (unsigned long long i = 0; i < ITERATIONS; i++)
    {
        seriesResult = seriesResult + (sign * (1.0 / denominator));
        denominator += 2.0;
        sign *= -1;
    }

    std::cout << std::setprecision(std::numeric_limits<long double>::digits10 + 2)
     << std::endl << "ssResult  = " << seriesResult << std::endl;

    approx_pi = 4 * (1 + seriesResult);

    // std::numeric_limits<long double>::digits10 + 2
    // is equivalent to
    // std::numeric_limits<long double>::max_digits10
    // in c++11
    // source: http://stackoverflow.com/a/554134
    std::cout << "sapprox pi = " << std::setprecision(std::numeric_limits<long double>::digits10 + 2) << approx_pi << std::endl;
}

void do_work(unsigned long long  start, unsigned long long  num_iterations)
{
    // int iterations = (num_iterations > 1) ? num_iterations-1 : num_iterations;
    char sign = (start % 2 == 0) ? -1 : 1;

    long double denominator = 3.0 + (2 * start);
    long double seriesResult = 0.0;

    // std::cout << "worker " << getpid() << std::endl
    //             << "start =  " << start << std::endl
    //             // << "iterations =  " << iterations << std::endl
    //             << "denom =  " << denominator << std::endl << std::flush;
    for (unsigned long long  i = 0; i < num_iterations; i++)
    {
        seriesResult = seriesResult + (sign * (1.0 / denominator));
        denominator += 2.0;
        sign *= -1;
    }
    // std::cout << std::setprecision(std::numeric_limits<long double>::digits10 + 2)
    //  << "sResult  = " << seriesResult << std::endl << std::flush;

    // TODO: may need to implement semaphore
    *global_result += seriesResult;
}

timespec diff(timespec start, timespec end)
{
    timespec temp;
    if ((end.tv_nsec-start.tv_nsec) < 0)
    {
        temp.tv_sec = end.tv_sec-start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec-start.tv_nsec;
    }
    else
    {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}


int main(int argc, char **argv)
{
    global_result = (long double*) mmap(NULL, sizeof *global_result, PROT_READ |
                                                PROT_WRITE,
                                                MAP_SHARED |
                                                MAP_ANONYMOUS,
                                                -1, 0);
    *global_result = 0.0;

    /*
    char buf[PATH_MAX];
    size_t len = ::readlink("/proc/self/exe", buf, sizeof(buf)-1);

    if (len != -1)
    {
        buf[len] = '\0';
    }
    else
    {
        // error
    }


    key_t key;
    int semid;
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = -1; // use the resource
    sb.sem_flg = SEM_UNDO;

    if ((key = ftok(buf, 'A') == -1)
    {
        // error msg
    }

    semid = semget(key, NUM_WORKERS, 0666 | IPC_CREAT);
    */

    timespec start;
    timespec finish;

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

    double iterations_per_worker = floor(ITERATIONS / NUM_WORKERS);
    unsigned long long  remaining_iterations = ITERATIONS - (iterations_per_worker * NUM_WORKERS);
    unsigned long long  start_iteration = 0;

    pid_t* pids = new pid_t[NUM_WORKERS-1];

    for (int i = 0; i < NUM_WORKERS - 1; i++)
    {
        pids[i] = fork();

        if (pids[i] == 0) // child
        {
            do_work(start_iteration, iterations_per_worker);
            return 0;
        }
        else if (pids[i] > 0) // parent
        {
            start_iteration += iterations_per_worker;
        }
        else
        {
            std::cout << "fork() failed" << std::endl;
            return -1;
        }
    }

    // parent process does last set of iterations
    do_work(start_iteration, iterations_per_worker + remaining_iterations);

    // force parent to wait for child processes to finish
    // error message if a child fails
    for (int i = 0; i < NUM_WORKERS-1; i++)
    {
        int status;
        while (-1 == waitpid(pids[i], &status, 0));

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        {
            std::cerr << "Process " << i << " (pid " << pids[i] << ") failed" << std::endl;
            return -2;
        }
    }

    long double approx_pi = 0.0;

    approx_pi = 4 * (1 + *global_result);

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &finish);

    // serial_test();
    std::cout << std::endl
    << "approx. pi = " << std::setprecision(std::numeric_limits<long double>::digits10 + 2) << approx_pi << std::endl;
    std::cout << "actual  pi = " << ACTUAL_PI << std::endl;

    std::cout << "elapsed time: " << diff(start,finish).tv_nsec << " ns" << std::endl;

    // TODO: check for failure
    munmap(global_result, sizeof *global_result);
    delete pids;

    return 0;
}