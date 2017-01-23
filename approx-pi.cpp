#include "approx-pi.h"

int main(int argc, char **argv)
{
    struct arguments arguments;

    // default argument options
    arguments.threads = 0;
    arguments.openmp = 0;
    arguments.output = 1;

    // execute argument parser
    argp_parse (&argp, argc, argv, 0, 0, &arguments);

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
                    threads[t_count] = std::thread(approx_pi_t, start_iteration, iters_per_worker, output, i, j);
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
                approx_pi(start_iteration, iters_per_worker, output, i);
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
    if (arguments.threads > 0)
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

            threads[t_count] = std::thread(approx_pi_t, start_iteration, iters_per_worker, output, num_processes-1, i);
            t_count++;

            start_iteration += iters_per_worker;
        }
        // join the threads to the parent process
        for (int i = 0; i < arguments.threads; i++)
        {
            threads[i].join();
        }
    }
    else
    {
        approx_pi(start_iteration, iters_per_worker + remaining_iterations, output, num_processes-1);
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

    // NOTE:
    // as the number of discrete workers increase, there is a loss of precision (last 3 decimals)
    // due to the addition done on the global_result variable

    // serial_test();

    std::cout
    << "approx. pi = " << std::setprecision(std::numeric_limits<long double>::max_digits10) << approx_pi_result << std::endl;
    std::cout << "actual  pi = " << ACTUAL_PI << std::endl;

    auto e_time = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::ofstream file;
    file.open("output/results.txt");
    file << "approx. pi = " << std::setprecision(std::numeric_limits<long double>::max_digits10) << approx_pi_result << "\n";
    file << "elapsed time: " << e_time << " milliseconds\n";
    file.close();

    std::cout << "elapsed time: " << e_time << " milliseconds" << std::endl;

    munmap(global_result, sizeof *global_result);
    sem_unlink(SEMAPHORE_NAME);
    delete pids;

    return 0;
}

void serial_test(unsigned long long iterations)
{

    long double seriesResult = 0.0;
    long double denominator = 3.0;
    long double approx_pi = 0.0;
    char sign = -1;

    for (unsigned long long i = 0; i < iterations; i++)
    {
        seriesResult = seriesResult + (sign * (1.0 / denominator));
        denominator += 2.0;
        sign *= -1;
    }

    std::cout << std::setprecision(std::numeric_limits<long double>::max_digits10)
     << std::endl << "ssResult  = " << seriesResult << std::endl;

    approx_pi = 4 * (1 + seriesResult);

    std::cout << "sapprox pi = " << std::setprecision(std::numeric_limits<long double>::max_digits10) << approx_pi << std::endl;
}

void approx_pi_t(unsigned long long start, unsigned long long num_iterations, bool output, int process, int thread)
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

    for (unsigned long long  i = 0; i < num_iterations; i++)
    {
        seriesResult = seriesResult + (sign * (1.0 / denominator));
        if (output)
        {
            file << "loop " << start + i << " : seriesResult = " << seriesResult << "\n";
        }
        denominator += 2.0;
        sign *= -1;
    }
    if (output)
    {
        file.close();
    }

    sem_t *semdes;
    if ((semdes = sem_open(SEMAPHORE_NAME, 0, 0644, 0)) == (void*) -1)
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

void approx_pi(unsigned long long start, unsigned long long num_iterations, bool output, int process)
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

    for (unsigned long long  i = 0; i < num_iterations; i++)
    {
        seriesResult = seriesResult + (sign * (1.0 / denominator));
        if (output)
        {
            file << "loop " << start + i << " : seriesResult = " << seriesResult << "\n";
        }
        denominator += 2.0;
        sign *= -1;
    }
    if (output)
    {
        file.close();
    }

    sem_t *semdes;
    if ((semdes = sem_open(SEMAPHORE_NAME, 0, 0644, 0)) == (void*) -1)
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