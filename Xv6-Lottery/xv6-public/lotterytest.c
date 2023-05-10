#include "types.h"
#include "user.h"

void loop(int n) {
    while(1);
}

int main(int argc, char* argv[]) {
    settickets(10);
    int is_parent;
    is_parent = fork();
    if(is_parent) {
        is_parent = fork();
        if(is_parent) {
            is_parent = fork();
            if(is_parent) {
                wait();
            } else {
                settickets(30);
                loop(3);
            }
            wait();
        } else {
            settickets(20);
            loop(2);
        }
        wait();
    } else {
        settickets(10);
        loop(1);
    }
    if(is_parent)wait();
    exit();
}