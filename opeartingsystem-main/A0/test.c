#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "./mypthread.h"
#include "./mypthread.c"


int fib(int i){
    if (i == 0){
        return 0;
    }
    if (i == 1){
        return 1;
    }
    
    return fib(i-1) + fib(i-2);

}

void* pow2(void* size){
    //printf("I MADE IT!!!!\n");
    //printf("x = %d\n", *(int*)size);
    int i = *(int*)size;
    int j = fib(i);
    *(int*)size = j;
    return size;
}

void* pow3(void* size){
    int j = 3000*30;
    return NULL;
}
pthread_t *thread;
int main(int argc, char **argv) {
    mypthread_t tid1;
    mypthread_t tid2;
    mypthread_t tid3;

    int variable;
    int *ptr = &variable;
    *ptr = 1;

    int variable2;
    int *ptr2 = &variable2;
    *ptr2 = 10;

    int variable3;
    int *ptr3 = &variable3;
    *ptr3 = 30;
// 
    int* size_ret1 = (int*) malloc(sizeof(int));
    int* size_ret2 = (int*) malloc(sizeof(int));
    int* size_ret3 = (int*) malloc(sizeof(int));
    int thread_num = 50;
    int i = 0;
    thread = (mypthread_t*)malloc(thread_num*sizeof(mypthread_t));
    for (i = 0; i < thread_num; i++){
        printf(" Created %u\n", thread[i]);
        mypthread_create(&thread[i],  NULL, pow3, ptr3);
        
    }
    printf("ere");
    for (i = 0; i < thread_num; i++){
       mypthread_join(thread[i], NULL);
        printf(" got value from thread %u\n", thread[i]);
    }


      printQueue(MTH->ready);
         printQueue(MTH->running);
         printQueue(MTH->blocked);
         printQueue(MTH->terminated);
    
	return 0;
}


