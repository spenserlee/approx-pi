#include <argp.h>

// argp globals
const char *argp_program_version = "approx-pi 0.1";
static char doc[] = "approx-pi -- a program to compare the performance between\
                    processes and threads, where the busy-work task is to \
                    approximate pi using the Taylor series.";

// a description of the accepted arguments
static char args_doc[] = "NUM_ITERATIONS NUM_PROCESSES";

// the options understood by this program
static struct argp_option options[] =
{
    {"threads",         't', "AMOUNT",      0,  "number of worker threads PER process"},
    {"openmp",          'm', "AMOUNT",      0,  "number of threads using the Open MP library"},
    {"output",          'o', 0,             0,  "disable worker I/O operations"},
    { 0 }
};

struct arguments
{
    char *args[2];
    int threads;
    int openmp;
    short output;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = (struct arguments *) state->input;

    switch (key)
    {
        case 't':
            arguments->threads = std::stoi(arg);
            break;
        case 'm':
            arguments->openmp = std::stoi(arg);
            break;
        case 'o':
            arguments->output = 0;
            break;

        case ARGP_KEY_ARG:
            if (state->arg_num > 2)    // too many arguments
            {
                argp_usage(state);
            }
            arguments->args[state->arg_num] = arg;
            break;

        case ARGP_KEY_END:
            if (state->arg_num < 2)     // not enough arguments
            {
                argp_usage(state);
            }
            if (arguments->threads )

            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

// the argument parser
static struct argp argp = { options, parse_opt, args_doc, doc };