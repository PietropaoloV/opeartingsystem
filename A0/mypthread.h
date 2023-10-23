// File:	mypthread_t.h

// List all group members' names:
// iLab machine tested on:

#ifndef MYTHREAD_T_H
#define MYTHREAD_T_H
#define _XOPEN_SOURCE 600

#define _GNU_SOURCE

/* in order to use the built-in Linux pthread library as a control for benchmarking, you have to comment the USE_MYTHREAD macro */
#define USE_MYTHREAD 1
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <stdatomic.h>

typedef uint mypthread_t;

typedef enum _t_pirority {
    LOW,
    MED,
    HIGH
}t_priority;

typedef enum t_state{
  RUNNING,
  WAITING,
  YIELDED,
  MUTEX_HOLD,
  MUTEX_UNLOCKED,
  BLOCKED,
  TERMINATED,
  SCHEDULER_RET,
  KEEP_RUNNING,
}t_state;

typedef enum _mode {
    RR = 0,
    PSJF = 1,
    MLFQ = 2
} mode;


typedef void* (*funcptr)(void*);
typedef struct threadControlBlock
{
  mypthread_t tid; //id
  t_priority priority; //priority
  char* name;  //name
  void* t_retval; // return value
  funcptr func_ptr;
  void* arg;
  ucontext_t* t_context; // context
  clock_t start_exec; // started execution time
  double total_exec; // total_execution time
  mypthread_t join_id; // join into which thread
} tcb;

typedef struct _TCBNODE {
    tcb* tcb;
    struct _TCBNODE* next;
} tcb_node;

typedef struct _TCBQUEUE {
   char* name;
   int size;
  struct _TCBNODE* front;
} tcb_queue;


/* add important states in a thread control block */

/* mutex struct definition */
typedef struct _mypthread_mutex_t mypthread_mutex_t;
typedef struct _MUTEXNODE mutex_node;
typedef struct _MutexHandler MH;
typedef struct _MUTEXHOLDQUEUE mutex_hold_node;

void initializeMutexQ(MH*);
void addToMutexList(MH *, mypthread_mutex_t *);
void addToMutexHoldQueue(mypthread_mutex_t *, mypthread_t);
void lockMutex(MH *, mypthread_mutex_t *, mypthread_t);
void destroyMutex(mypthread_mutex_t *);
int isOnHoldQueueById(mypthread_mutex_t *, mypthread_t);
mypthread_mutex_t * getMutexIfExists(mutex_node *, mypthread_mutex_t *);
void removeFromMutexHoldQueue(mypthread_mutex_t *, mypthread_t);
void lockMutexWithNextWaitingThread(mypthread_mutex_t *);
int isOnMutexHold(tcb *);

int testMutexQ();

typedef struct _mypthread_mutex_t
{
  int flag; //id
  int guard; //lock=1/unlock=0
  tcb *owner; //thread that owns it
  tcb_queue *hold_queue;
    
}mypthread_mutex_t;

typedef struct _MUTEXNODE
{
  mypthread_mutex_t* mutex;
  mutex_node* next; 
    
}mutex_node;



// Feel free to add your own auxiliary data structures (linked list or queue etc...)
typedef struct _Scheduler{
   tcb_node* s_tcb_node;
   struct _TCBQUEUE* running;
   struct _TCBQUEUE* medium;
   struct _TCBQUEUE* low;
   struct _TCBQUEUE* ready;
   struct _TCBQUEUE* resource;
   struct _TCBQUEUE* blocked;
   struct _TCBQUEUE* terminated;
   tcb_node* current;
}TH;

/* include lib header files that you need here: */

// Queue Declarations
tcb_node* createTCBNode(tcb* tcb);
void enqueue(tcb_node* tcb, tcb_queue* q);
tcb_node* dequeue(tcb_queue* q);
tcb* peek(tcb_queue* q);
void printQueue(tcb_queue* q);
tcb_node* searchQueue(tcb_queue* q, mypthread_t tid);
tcb_node* searchQueueAndRemove(tcb_queue* q, mypthread_t tid);
int swapQueues(tcb_queue* source, tcb_queue* destination, mypthread_t tid);

/* Function Declarations: */

tcb_queue* createQueue(char*);
void initializeTH(TH*);
void getTime(struct timespec);
tcb* setupThread(ucontext_t*, mypthread_t);
int isEmpty(tcb_queue*);
int transferQueue(tcb_queue*, tcb_queue*);

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* current thread voluntarily surrenders its remaining runtime for other threads to use */
int mypthread_yield();

/* terminate a thread */
void mypthread_exit(void *value_ptr);

/* blocked for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr);

/* initialize a mutex */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire a mutex (lock) */
int mypthread_mutex_lock(mypthread_mutex_t *mutex);

/* release a mutex (unlock) */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex);

/* destroy a mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex);





#define HIGH_EXEC_TIMEOUT 50
#define MEDIUM_EXEC_TIMEOUT 150
#define LOW_EXEC_TIMEOUT 300
#define T_STACK_SIZE 1048576

#ifdef USE_MYTHREAD
#define pthread_t mypthread_t
#define pthread_mutex_t mypthread_mutex_t
#define pthread_create mypthread_create
#define pthread_exit mypthread_exit
#define pthread_join mypthread_join
#define pthread_mutex_init mypthread_mutex_init
#define pthread_mutex_lock mypthread_mutex_lock
#define pthread_mutex_unlock mypthread_mutex_unlock
#define pthread_mutex_destroy mypthread_mutex_destroy
extern unsigned int s_tid;  //scheduler id
extern TH* MTH; // scheduler
extern MH* MQ;
extern ucontext_t ctx_main; // main context
extern ucontext_t ctx_handler; // context handler
extern mode schedMode; //Which scheduler we use
extern int isSchedulerNotStarted;
extern int isMutexQNotAllocated;
extern struct sigaction act_timer;
extern struct itimerval timer;
extern int load_balance;
#endif

#endif


