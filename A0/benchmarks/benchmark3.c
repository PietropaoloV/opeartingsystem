#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "../mypthread.h"

/* Global variables */
int thread_num;
pthread_t *thread;

/* A CPU-bound task tzo do vector multiplication */
void call_api(void* arg) {
	dummyApi();
	pthread_exit(NULL);
}

// A dummy function to simulate a long running api with unknown IO times
void dummyApi(){
	int r = rand() % 10;
	for(int i=0; i < 100000000*r; i++) continue;
}

void verify(int num){
	struct timespec start, end;
	clock_gettime(CLOCK_REALTIME, &start);

	for (int i = 0; i < num; i++)
		call_api(1);

	clock_gettime(CLOCK_REALTIME, &end);
        printf("Sequential running time: %lu micro-seconds\n", 
	       (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);
}

int main(int argc, char **argv) {
	int num_api_calls;

	if (argv[1] < 1) {
			printf("enter a valid thread number\n");
			return 0;
	} else {
			num_api_calls = atoi(argv[1]);
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
	thread = (pthread_t*)malloc(num_api_calls*sizeof(pthread_t));

	struct timespec start, end;
        clock_gettime(CLOCK_REALTIME, &start);

	for (int i = 0; i < num_api_calls; i++)
		pthread_create(&thread[i], NULL, &call_api, NULL);

	for (int i = 0; i < num_api_calls; i++)
		pthread_join(thread[i], NULL);

	clock_gettime(CLOCK_REALTIME, &end);
        printf("Pthreads running time: %lu micro-seconds\n", 
	       (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);
	verify(num_api_calls);

	// Free memory on Heap
	free(thread);
	printf("\n7");
	return 0;
}
