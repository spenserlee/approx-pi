#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <limits>

#define ITERATIONS 1000000000
#define ACTUAL_PI "3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679821"

int main(int argc, char **argv)
{

    // pi       = 4 * ( 1 - 1/3 + 1/5 - 1/7 + 1/9 - 1/11 ... )
    // let y    = 1/3 + 1/5 - 1/7 + 1/9 - 1/11 ...
    // let z    = ( 1 + y )
    // pi       = 4 * z

    long double pi = 0;
    long double y = 0;
    long double z = 0;

    long double x = 3.0;
    char sign = -1;

    for (int i = 0; i < ITERATIONS; i++)
    {
        y = y + (sign * (1.0 / x));

        x += 2.0;
        sign *= -1;
    }

    z = 1.0 + y;
    pi = z * 4;

    std::cout << "approx. pi = " << std::setprecision(std::numeric_limits<long double>::digits10 + 2) << pi << std::endl;
    std::cout << "actual  pi = " << ACTUAL_PI << std::endl;


    return 0;
}