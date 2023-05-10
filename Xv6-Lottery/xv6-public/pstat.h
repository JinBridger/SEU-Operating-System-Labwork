#pragma once
#include "param.h"

struct pstat{
    int inuse[NPROC];
    int tickets[NPROC];
    int pid[NPROC];
    int ticks[NPROC];
};