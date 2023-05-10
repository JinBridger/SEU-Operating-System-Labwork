#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "mmu.h"

int main(int argc, char *argv[]) {
    char* val = sbrk(0);
    sbrk(PGSIZE);

    *val = 5;
    printf(1, "Start at %d\n", *val);

    mprotect((void*)val, 1);
    munprotect((void*)val, 1);

    *val = 10;
    printf(1, "Now is %d\n", *val);

    exit();
}