#include "random.h"

unsigned int ran_A = 1103515245;
unsigned int ran_B = 12345;
unsigned int ran_status = 5167495;

void srand(unsigned int seed) {
    ran_status = seed;
}

unsigned int rand() {
    unsigned long long tmp = ran_A * ran_status + ran_B;
    ran_status = (unsigned int)tmp;
    return ran_status;
}
