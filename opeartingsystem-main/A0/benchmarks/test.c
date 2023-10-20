#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../mypthread.h"
#include "../mypthread.c"
// unsigned int tid = 5;
// MTH* Master;
// ucontext_t* ctx_main;

// typedef struct _JUNK {
//     int x;
//     char y;
// }t_struct;

// void* t1(void* arg){
//     printf("Thread has been created\n");
//     int x = 10;
//     printf("x = %d\n", x);
//     return NULL;
// }

int main(int argc, char **argv) {
    startScheduler();
    
    
	return 0;
}
