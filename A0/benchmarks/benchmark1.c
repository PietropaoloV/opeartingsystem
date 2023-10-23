#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include "../mypthread.h"

#define DEFAULT_THREAD_NUM 2
#define ARRAY_SIZE 100000

/* Global variables */
pthread_mutex_t   mutex;
int thread_num;
int* counter;
pthread_t *thread;
int ar_par[ARRAY_SIZE];
int ar_verify[ARRAY_SIZE];

typedef struct par_args{
    int arg1;
    int arg2;

} pargs;

/* A CPU-bound task to do vector multiplication */
// A merge sort with 2 threads
void parallel_merge(int arr[], int l, int m, int r)
{
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;
 
    // Create temp arrays
    pthread_mutex_lock(&mutex);
    int **L = malloc(n1 * sizeof *L);
    int **R = malloc(n2 * sizeof *R);
    pthread_mutex_unlock(&mutex);   
 
    // Copy data to temp arrays L[] and R[]
    for (i = 0; i < n1; i++){
        pthread_mutex_lock(&mutex);
L[i] = arr[l + i];
pthread_mutex_unlock(&mutex);   
    }
        
    for (j = 0; j < n2; j++){
        pthread_mutex_lock(&mutex);
        R[j] = arr[m + 1 + j];

        pthread_mutex_unlock(&mutex);   

    }
 
    // Merge the temp arrays back into arr[l..r
    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            pthread_mutex_lock(&mutex);
            arr[k] = L[i];
            pthread_mutex_unlock(&mutex);   
            i++;
        }
        else {
            pthread_mutex_lock(&mutex);
            arr[k] = R[j];
            pthread_mutex_unlock(&mutex);
            j++;
        }
        k++;
    }
 
    // Copy the remaining elements of L[],
    // if there are any
    while (i < n1) {
        pthread_mutex_lock(&mutex);
        arr[k] = L[i];
        pthread_mutex_unlock(&mutex);
        i++;
        k++;
    }
 
    // Copy the remaining elements of R[],
    // if there are any
    while (j < n2) {
        pthread_mutex_lock(&mutex);
        arr[k] = R[j];
        pthread_mutex_unlock(&mutex);
        j++;
        k++;
    }
    free(L);
    free(R);
}
void merge(int arr[], int l, int m, int r)
{
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;
 
    // Create temp arrays
    int **L = malloc(n1 * sizeof *L);
    int **R = malloc(n2 * sizeof *R);
    // int L[n1];
    // int R[n2];
 
    // Copy data to temp arrays L[] and R[]
    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];
 
    // Merge the temp arrays back into arr[l..r
    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        }
        else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }
 
    // Copy the remaining elements of L[],
    // if there are any
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }
 
    // Copy the remaining elements of R[],
    // if there are any
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
    // free(L);
    // free(R);
}

void parallelMergeSort(int arr[], int l, int r)
{
    if (l < r) {
        int m = l + (r - l) / 2;
        parallelMergeSort(arr, l, m);
        parallelMergeSort(arr, m + 1, r);
 
        parallel_merge(arr, l, m, r);
    }
}

void parallel_worker(pargs *args){
    parallelMergeSort(ar_par, args->arg1, args->arg2);
}

void parallel_driver(){
    int l = 0;
    int r = ARRAY_SIZE - 1;
    int m = l + (r - l) / 2;
    pargs l_args = {l, m};
    pargs r_args = {m+1, r};

    pthread_create(&thread[0], NULL, &parallel_worker, &l_args);
    pthread_create(&thread[1], NULL, &parallel_worker, &r_args);

    pthread_join(thread[0], NULL);
    pthread_join(thread[1], NULL);

    parallel_merge(ar_par, l, m, r);
}
 
// l is for left index and r is right index of the
// sub-array of arr to be sorted
void mergeSort(int arr[], int l, int r)
{
    if (l < r) {
        int m = l + (r - l) / 2;
        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);
 
        merge(arr, l, m, r);
    }
}
 
// Function to print an array
void printArray(int A[], int size)
{
    int i;
    int max_print = 100;
    if(max_print > size) max_print = size;
    for (i = 0; i < max_print; i++)
        printf("%d ", A[i]);
    printf("\n");
}

void verify() {
    mergeSort(ar_verify, 0, ARRAY_SIZE - 1);
    printArray(ar_verify, ARRAY_SIZE);
}

int main(int argc, char **argv) {
    
    int i = 0;
    thread_num = 2;

    if(argc > 1){
		if(atoi(argv[1]) == 0 || atoi(argv[1]) == 1 || atoi(argv[1]) == 2)
		setScheduler(atoi(argv[1]));
	else 
		printf("\nPlease select the correct scheduler mode");
	}
	else{
		setScheduler(-1);
	}

    // initialize pthread_t
    thread = (pthread_t*)malloc(thread_num*sizeof(pthread_t));

    // initialize data array
    srand(time(NULL)); 
    for (i = 0; i < ARRAY_SIZE; ++i) {
          // Initialization, should only be called once.
          int r = rand() % ARRAY_SIZE;
        // int r;
        // while(1){
        //  r = rand() % ARRAY_SIZE;
        //  if(r != 0) continue;
        // }
        ar_par[i] = r;
        ar_verify[i] = r;
    }

    printf("\nNormal Sort: \n");
    verify();
    printf("\n");

    pthread_mutex_init(&mutex, NULL);

    struct timespec start, end;
        clock_gettime(CLOCK_REALTIME, &start);

    parallel_driver();

    clock_gettime(CLOCK_REALTIME, &end);
        printf("running time: %lu micro-seconds\n", 
           (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000);
    printf("\nTwo Thread Parallel Sort With Mutexes: \n");
    printArray(ar_par, ARRAY_SIZE);

    pthread_mutex_destroy(&mutex);


    // Free memory on Heap
    free(thread);
    free(counter);
    return 0;
}
