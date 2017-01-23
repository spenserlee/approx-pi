/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: approx-pi.cpp - This file is the main program for calculating pi.
--
-- PROGRAM: approx-pi
--
-- FUNCTIONS:
--      main
--      approx_pi_t
--      approx_pi
--
-- DATE: January 23, 2017
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Spenser Lee
--
-- PROGRAMMER: Spenser Lee
--
-- NOTES:
-- This file contains the main program. It takes the command line input then creates a user specified number
-- of worker processes and threads.
--
-- This program's primary objective is to act as a busy-work task as a benchmark for comparing the performance
-- of threads and processes.
--
-- NOTE:
-- as the number of discrete workers increase, there is a loss of precision (last 3 decimals)
-- due to the addition done on the global_result variable.
--
-- The forumla used to approximate Pi is the Taylor series. It is understood that using the Taylor series to
-- approximate pi is very inefficient. For more information on the Taylor series to calculate Pi see:
-- https://www.math.hmc.edu/funfacts/ffiles/30001.1-3.shtml
-- http://www.geom.uiuc.edu/~huberty/math5337/groupe/expresspi.html?
----------------------------------------------------------------------------------------------------------------------*/

#include "approx-pi.h"

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: main
--
-- DATE: January 23 2017
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Spenser Lee
--
-- PROGRAMMER: Spenser Lee
--
-- INTERFACE: main(int argc, char **argv)
--
-- RETURNS: int
--
-- NOTES:
-- Entry point of the program. After executing the argp argument parser, the program will initialize a semaphore
-- and mapped memory segment. Then, based on the user arguments (number of processes/threads), it will
-- calculate how much work each process/thread needs to do. Then it will cleanup and create output the output
-- directory.
--
-- Finally, it will spawn off all of the worker processes/threads, each of which will be writing their result
-- work into the mapped memory segment once complete. Then the program will output its results and end.
----------------------------------------------------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
    struct arguments arguments;

    // default argument options
    arguments.threads = 0;
    arguments.openmp = 0;
    arguments.output = 1;

    // execute argument parser
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    unsigned long long num_iters = std::stoull(arguments.args[0]);
    unsigned int num_processes = std::stoi(arguments.args[1]);

    // create named semaphore
    sem_t *mysem;
    if ((mysem = sem_open(SEMAPHORE_NAME, O_CREAT, 0644, 1)) == (void*) -1) {
        std::cout << "sem_open() failed" << std::endl;
        return -1;
    }

    // allocate memory map for the global result accessed by all workers
    global_result = (long double*) mmap(NULL, sizeof *global_result, PROT_READ |
                                                PROT_WRITE,
                                                MAP_SHARED |
                                                MAP_ANONYMOUS,
                                                -1, 0);
    *global_result = 0.0;


    // if there are no threaded workers, total workers is just number of processes
    // otherwise its is the product of threads per process
    int total_workers = (arguments.threads == 0 ? num_processes : num_processes * arguments.threads);
    double iters_per_worker = floor(num_iters / total_workers);
    unsigned long long remaining_iterations = num_iters - (iters_per_worker * total_workers);
    unsigned long long start_iteration = 0;

    bool output = (arguments.output == 1 ? true : false);

    std::cout << "Total Iterations              = " << num_iters << std::endl
              << "Total Workers                 = " << total_workers << std::endl
              << "Iterations Per Worker         = " << iters_per_worker << std::endl
              << "Process Workers               = " << num_processes << std::endl
              << "Thread Workers Per Process    = " << arguments.threads << std::endl
              << "File Output                   = " << output << std::endl << std::endl;

    if (output)
    {
        if (stat("./output", &st) == -1)
        {
            mkdir("./output", 0775);
        }
        if (stat("./output/results.txt", &st) != -1)
        {
            system("exec rm output/worker-output*");
            system("exec rm output/results.txt");
        }
    }

    std::thread threads[arguments.threads];
    int t_count = 0;

    pid_t* pids = new pid_t[num_processes-1];

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    for (unsigned int i = 0; i < num_processes - 1; i++)
    {
        pids[i] = fork();

        if (pids[i] == 0) // child
        {
            if (arguments.threads > 0)
            {
                for (int j = 0; j < arguments.threads; j++)
                {
                    threads[t_count] = std::thread(approx_pi_t, start_iteration, iters_per_worker, num_iters, output, i, j);
                    t_count++;
                    start_iteration += iters_per_worker;
                }
                // join the threads to the child process
                for (int i = 0; i < arguments.threads; i++)
                {
                    threads[i].join();
                }
            }
            else
            {
                approx_pi(start_iteration, iters_per_worker, num_iters, output, i);
            }
            return 0;
        }
        else if (pids[i] > 0) // parent process
        {
            if (arguments.threads > 0)
            {
                start_iteration += (iters_per_worker * arguments.threads);
            }
            else
            {
                start_iteration += iters_per_worker;
            }
        }
        else if (pids[i] == -1)
        {
            std::cout << "fork() failed" << std::endl;
            return -2;
        }
    }

    // parent process does last set of iterations

    if (arguments.threads > 0)  // using threads
    {
        for (int i = 0; i < arguments.threads; i++)
        {
            if (i == arguments.threads-1)
            {
                num_iters = iters_per_worker + remaining_iterations;
            }
            else
            {
                num_iters = iters_per_worker;
            }

            threads[t_count] = std::thread(approx_pi_t, start_iteration, iters_per_worker, num_iters, output, num_processes-1, i);
            t_count++;

            start_iteration += iters_per_worker;
        }
        // join the threads to the parent process
        for (int i = 0; i < arguments.threads; i++)
        {
            threads[i].join();
        }
    }
    else    // using processes
    {
        approx_pi(start_iteration, iters_per_worker + remaining_iterations, num_iters, output, num_processes-1);
    }
    sem_close(mysem);

    // force parent to wait for child processes to finish
    // error message if a child fails
    for (unsigned int i = 0; i < num_processes-1; i++)
    {
        int status;
        while (-1 == waitpid(pids[i], &status, 0));

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        {
            std::cout << "Process " << i << " (pid " << pids[i] << ") failed" << std::endl;
            return -3;
        }
    }

    long double approx_pi_result = 0.0;

    approx_pi_result = 4 * (1 + *global_result);

    std::chrono::high_resolution_clock::time_point finish = std::chrono::high_resolution_clock::now();

    std::cout << "approx. pi = "
              << std::setprecision(std::numeric_limits<long double>::max_digits10)
              << approx_pi_result
              << std::endl;
    std::cout << "actual  pi = "
              << ACTUAL_PI
              << std::endl;

    auto e_time = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::ofstream file;
    file.open("output/results.txt");
    file << "approx. pi = " << std::setprecision(std::numeric_limits<long double>::max_digits10) << approx_pi_result << "\n";
    file << "elapsed time: " << e_time << " milliseconds\n";
    file.close();

    std::cout << "\nelapsed time: " << e_time << " milliseconds" << std::endl;

    munmap(global_result, sizeof *global_result);
    sem_unlink(SEMAPHORE_NAME);
    delete pids;

    return 0;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: approx_pi_t
--
-- DATE: January 23 2017
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Spenser Lee
--
-- PROGRAMMER: Spenser Lee
--
-- INTERFACE: void approx_pi_t(
                    unsigned long long start,
                    unsigned long long num_iters,
                    unsigned long long total_iters,
                    bool output,
                    int process,
                    int thread)
--
-- RETURNS: void
--
-- NOTES:
-- This is the thread worker function. The important input is the start offset and number of worker iterations.
--
-- Each worker is given an offset within the Taylor series, so each worker is a section of 'y':
-- pi       = 4 * ( 1 - 1/3 + 1/5 - 1/7 + 1/9 - 1/11 ... )
-- let y    = - 1/3 + 1/5 - 1/7 + 1/9 - 1/11 + 1/13 - 1/15 + 1/17 ...)
-- let z    = ( 1 + y )
-- pi       = 4 * z
-- In this case, 'y' has been named 'global_result'.
----------------------------------------------------------------------------------------------------------------------*/
void approx_pi_t(
    unsigned long long start,
    unsigned long long num_iters,
    unsigned long long total_iters,
    bool output,
    int process,
    int thread)
{
    char sign = (start % 2 == 0) ? -1 : 1;
    long double denominator = 3.0 + (2 * start);
    long double seriesResult = 0.0;
    std::ofstream file;

    if (output)
    {
        std::string file_name = "output/worker-output-p" + std::to_string(process) + "-t" + std::to_string(thread) + ".txt";
        file.open(file_name);
    }

    for (unsigned long long i = 0; i < num_iters; i++)
    {
        seriesResult = seriesResult + (sign * (1.0 / denominator));
        if (output)
        {
            if (total_iters > 100000000)    // safeguard to prevent huge output file size
            {
                if (i % 10000 == 0)
                {
                   file << "loop " << start + i << " : seriesResult = " << seriesResult << "\n";
                }
            }
            else {
                file << "loop " << start + i << " : seriesResult = " << seriesResult << "\n";
            }
        }
        denominator += 2.0;
        sign *= -1;
    }
    if (output)
    {
        file.close();
    }

    sem_t *semdes;
    if ((semdes = sem_open(SEMAPHORE_NAME,
        0, 0644, 0)) == (void*) -1)
    {
        std::cout << "sem_open() in work failed" << std::endl;
    }

    // wait until semaphore is available then lock
    if (!sem_wait(semdes))
    {
        *global_result += seriesResult;
        sem_post(semdes);   // release semaphore lock
    }
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: approx_pi
--
-- DATE: January 23 2017
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Spenser Lee
--
-- PROGRAMMER: Spenser Lee
--
-- INTERFACE: void approx_pi(
                    unsigned long long start,
                    unsigned long long num_iters,
                    unsigned long long total_iters,
                    bool output,
                    int process)
--
-- RETURNS: void
--
-- NOTES:
-- This is the process worker function. The important input is the start offset and number of worker iterations.
--
-- Each worker is given an offset within the Taylor series, so each worker is a section of 'y':
-- pi       = 4 * ( 1 - 1/3 + 1/5 - 1/7 + 1/9 - 1/11 ... )
-- let y    = - 1/3 + 1/5 - 1/7 + 1/9 - 1/11 + 1/13 - 1/15 + 1/17 ...)
-- let z    = ( 1 + y )
-- pi       = 4 * z
-- In this case, 'y' has been named 'global_result'.
----------------------------------------------------------------------------------------------------------------------*/
void approx_pi(
    unsigned long long start,
    unsigned long long num_iters,
    unsigned long long total_iters,
    bool output,
    int process)
{
    char sign = (start % 2 == 0) ? -1 : 1;
    long double denominator = 3.0 + (2 * start);
    long double seriesResult = 0.0;
    std::ofstream file;

    if (output)
    {
        std::string file_name = "output/worker-output-p" + std::to_string(process) + ".txt";
        file.open(file_name);
    }

    for (unsigned long long i = 0; i < num_iters; i++)
    {
        seriesResult = seriesResult + (sign * (1.0 / denominator));
        if (output)
        {
            if (total_iters > 100000000)    // safeguard to prevent huge output file size
            {
                if (i % 10000 == 0)
                {
                   file << "loop " << start + i << " : seriesResult = " << seriesResult << "\n";
                }
            }
            else {
                file << "loop " << start + i << " : seriesResult = " << seriesResult << "\n";
            }
        }
        denominator += 2.0;
        sign *= -1;
    }
    if (output)
    {
        file.close();
    }

    sem_t *semdes;
    if ((semdes = sem_open(SEMAPHORE_NAME,
        0, 0644, 0)) == (void*) -1)
    {
        std::cout << "sem_open() in work failed" << std::endl;
    }

    // wait until semaphore is available then lock
    if (!sem_wait(semdes))
    {
        *global_result += seriesResult;

        sem_post(semdes);   // release semaphore lock
        sem_close(semdes);  // disassociate the semaphore with this particular process
    }
}