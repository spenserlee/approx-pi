/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: approx-pi.h - Header file for program.
--
-- PROGRAM: approx-pi
--
-- FUNCTIONS:
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
-- This file contains the header files and definitions for the main program.
----------------------------------------------------------------------------------------------------------------------*/

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

