#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../mypthread.h"

#define DEFAULT_THREAD_NUM 2
#define ARRAY_SIZE 500

/* Global variables */
pthread_mutex_t counter_mutex, data_mutex;
int thread_num;
int* counter;
pthread_t *thread;
int *thread_counter;
int ar_parr[ARRAY_SIZE][ARRAY_SIZE];
int ar_parr_map_store[ARRAY_SIZE];
int ar_verify[ARRAY_SIZE][ARRAY_SIZE];
int reduced_sum = 0;
int reduced_verified_sum = 0;
int res = 0;

void executor(void *func, int arg){
	// printf("\nARG: %d", arg);
	pthread_mutex_lock(&counter_mutex);
	++*thread_counter;
	pthread_create(&thread[*thread_counter], NULL, func, &counter[arg]);
	pthread_mutex_unlock(&counter_mutex);
}

void map(int *pos){
	// printf("\nPOS: %d", *pos);
	for(int i=0; i < ARRAY_SIZE; i++){
		pthread_mutex_lock(&data_mutex);
		ar_parr[*pos][i] = ar_parr[*pos][i] * 2;
		pthread_mutex_unlock(&data_mutex);
	}
}

void reduce(int *pos){
	for(int i=0; i < ARRAY_SIZE; i++){
		pthread_mutex_lock(&data_mutex);
		reduced_sum += ar_parr[*pos][i];
		pthread_mutex_unlock(&data_mutex);
	}
}

void verify() {
	for(int i=0; i < ARRAY_SIZE; i++){
		for(int j=0; j < ARRAY_SIZE; j++){
			ar_verify[i][j] = ar_verify[i][j] * 2;
		}
	}
	for(int i=0; i < ARRAY_SIZE; i++){
		for(int j=0; j < ARRAY_SIZE; j++){
			reduced_verified_sum += ar_verify[i][j];
		}
	}
	printf("verified res is: %d\n", reduced_verified_sum);
}

void doNothing(){

}

int main(int argc, char **argv) {
	
	int test = ARRAY_SIZE;
// printf("\nArray SIZE: %d", test);
	if (argc == 1) {
		thread_num = DEFAULT_THREAD_NUM;
	} else {
		if (argv[1] < 1) {
			printf("enter a valid thread number\n");
			return 0;
		} else {
			thread_num = atoi(argv[1]);
		}
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

	// // initialize counter
	counter = (int*)malloc(2*ARRAY_SIZE*sizeof(int));
	thread_counter = (int*)malloc(sizeof(int));
	// // Thread Counter is 0 indexed
	*thread_counter = 0;

	for (int i = 0; i < ARRAY_SIZE; ++i)
{		counter[i*2] = i;
		counter[(i*2)+1] = i;
}
	// initialize pthread_t
	thread = (pthread_t*)malloc(10000*sizeof(pthread_t));

	// initialize data array
	for (int j = 0; j < ARRAY_SIZE; ++j) {
		for (int k = 0; k < ARRAY_SIZE; ++k) {
			ar_parr[j][k] = j + k;
			ar_verify[j][k] = j + k;
		}
	}

	pthread_mutex_init(&counter_mutex, NULL);
	pthread_mutex_init(&data_mutex, NULL);

	int doNothingThreadCounter = 0;
	pthread_create(&thread[doNothingThreadCounter], NULL, &doNothing, thread_counter);

	struct timespec start, end;
        clock_gettime(CLOCK_REALTIME, &start);
	
	for(int pos=0; pos < ARRAY_SIZE; pos++){
		executor(map, 2*pos);
		executor(reduce,2*pos+1);
	}
	for(int k=0; k < *thread_counter; k++){
		
		pthread_join(thread[k], NULL);
	}

	// pthread_join(thread[doNothingThreadCounter], NULL);


	clock_gettime(CLOCK_REALTIME, &end);
        printf("running time: %lu micro-seconds\n", 
	       (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);
	printf("res is: %d\n", reduced_sum);
	// printf("\n\nVerify Arry:\n");
	// for (int j = 0; j < ARRAY_SIZE; ++j) {
	// 	for (int k = 0; k < ARRAY_SIZE; ++k) {
	// 		printf("%d ",ar_verify[j][k]);
	// 	}
	// 	printf("\n");
	// }
	// printf("\n\nParr Arry:\n");
	// for (int j = 0; j < ARRAY_SIZE; ++j) {
	// 	for (int k = 0; k < ARRAY_SIZE; ++k) {
	// 		printf("%d ",ar_parr[j][k]);
	// 	}
	// 	printf("\n");
	// }

	pthread_mutex_destroy(&counter_mutex);
	pthread_mutex_destroy(&data_mutex);

	verify();

	// Free memory on Heap
	free(thread);
	free(counter);
	return 0;
}
