#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int main(int argc, char *argv[]) {

    uint* nullp = (uint*) 0;
    printf(1, "%x %x\n", nullp, *nullp);

    exit();
}