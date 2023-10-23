#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "../mypthread.h"

/* Global variables */
int thread_num;
pthread_t *thread;
long turnaround_total = 0;

// Benchmark 4 tests a comaprison between RR and MLFQ
// This benchmark has two threads that run. 
// T1-> ~1000ms runtime
// T2-> ~10000ms runtime
// With MLFQ, turnaround time for T1 should be way less than RR

// The for loop is tunes to be ~1000ms
void T1(struct timespec *start) {
	// printf("\nT1");
	struct timespec end;
	for(long i=0; i < 48500000; i++) continue;
	clock_gettime(CLOCK_REALTIME, &end);
	turnaround_total += (long)(end.tv_sec - start->tv_sec) * 1000 + (end.tv_nsec - start->tv_nsec) / 1000000;
}

// The for loop is tuned to be ~10000ms
void T2(struct timespec *start){
	struct timespec end;
	for(long i=0; i < 3850000000; i++) continue;
		clock_gettime(CLOCK_REALTIME, &end);
        printf("T2 running time: %lu micro-seconds\n", 
	       (end.tv_sec - start->tv_sec) * 1000 + (end.tv_nsec - start->tv_nsec) / 1000000);
}

int main(int argc, char **argv) {

	if (argv[1] < 1) {
			printf("enter a valid thread number\n");
			return 0;
	} else {
			thread_num = atoi(argv[1]);
	}
	if(argc > 2){
		if(atoi(argv[2]) == 0 || atoi(argv[2]) == 1 || atoi(argv[2]) == 2)
		setScheduler(atoi(argv[2]));
	else 
		printf("\nPlease select the correct scheduler mode");
	}
	else{
		setScheduler(-1);
	}
	// initialize pthread_t
	thread = (pthread_t*)malloc(thread_num*sizeof(pthread_t));

	struct timespec start;
	clock_gettime(CLOCK_REALTIME, &start);

	// printf("\nThread Num: %d", thread_num);
	for (int i = 1; i < thread_num+1; i++)
		pthread_create(&thread[i], NULL, &T1, &start);
	
	pthread_create(&thread[0], NULL, &T2, &start);

	for (int i = 1; i < thread_num+1; i++)
		pthread_join(thread[i], NULL);

	pthread_join(thread[0], NULL);

	
+	printf("\nAverage Turnaround time for T1 = %d\n", turnaround_total/thread_num);

	// Free memory on Heap
	free(thread);
	return 0;
}
