// File:	mypthread.c

// List all group members' names:
// iLab machine tested on:

#include "mypthread.h"


//Global Vars


extern unsigned int tid;  //scheduler id
extern TH* MTH; // scheduler
extern ucontext_t ctx_main; // main context
extern ucontext_t ctx_handler; // context handler
extern mode schedMode; //Which scheduler we use




static initTH(TH* MTH) {
	TH* MTH = (TH*) malloc(sizeof(TH));
	initializeTH(MTH);
}

static void start() {
	if (MTH == null) {
		initTH(MTH);
	}
}




/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg)
{
	//Creates New Thread
	//Setup
		//Set OS bit



	//Execution
		Give the thread new state 
		init all variables
			make context 
			calulate init time
			set tid


	//Teardown
		//Go back to thread mode


	return 0;
};

/* current thread voluntarily surrenders its remaining runtime for other threads to use */
int mypthread_yield()
{
	//Setup
		set os bit
		
	//Execution
		call the scheduler with value of YIELDING

	//Teardown
		set bit to thread mode

	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr)
{
	//Setup
		set os bit

	//Execution
		check all locks are unlocked 
		check that all joined threads are moved to queue again
		call the scheduler with value of TERMINATING 


	//Teardown
		yield
	
	return;
};


/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr)
{
	//Setup
		set os bit

	//Execution
		set join id and put thread on a join queue where it will check upon a threads completion
		set status to BLOCKING
		

	//Teardown
		yield

	return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
	//setup
	set os bit

	//Execution
		create new mutex add it to mutex queue

	//Teardonw
	return back to user mode

	return 0;
};

/* aquire a mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex)
{
	//setup
	set os bit

	//Execution
		check mutex exists and set mutex lock id to current thread if mutex is unlocked
		then go to tear down

		if mutex lock is locekd change current thread to BLOCKED
		and yield else teardown


	//Teardown
	return back to user mode
	
		// use the built-in test-and-set atomic function to test the mutex
		// if the mutex is acquired successfully, return
		// if acquiring mutex fails, put the current thread on the blocked/waiting list and context switch to the scheduler thread
		
		return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex)
{
	//setup
	set os bit

	//Execution
		see if current owner is able to unlock because their id is on the mutex
		then go to tear down

		if mutex lock id is already taken and you dont own it then change current thread to TERMINATING
		and yield else teardown

	
	//Teardown
	return back to user mode

	return 0;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex)
{
	//setup
	set os bit

	//Execution
		see if no thread id is associated with the mutex if it is clear then free.

		if its not taken then exit for now

	
	//Teardown
	return back to user mode

	return 0;
};

static void setupSwapContexts() {
  if(getcontext(&ctx_handler) == -1){
    fprintf(stderr, "Could not get main context in scheudler context to scheduled thread. Error msg: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  ctx_handler.uc_stack.ss_sp = (void*)malloc(T_STACK_SIZE);
  
  if(!ctx_handler.uc_stack.ss_sp){
    printf("Malloc didn't work :(\n");
    exit(EXIT_FAILURE);
  }
  
  ctx_handler.uc_stack.ss_size = T_STACK_SIZE; 
  ctx_handler.uc_link = &ctx_main;

  
}

static void scheduleNextThread(){
    printf("no current thread set, setting current thread\n");
    MTH->current = dispatcher(MTH);
    	if(MTH->current == NULL){
      		printf("No more threads to be executed\n");
     		exit(0);
   		}
    	else if(swapcontext(&ctx_handler, &MTH->current->t_context) == -1){
      		fprintf(stderr, "Could not swap context to scheduled thread. Error msg: %s\n", strerror(errno));
      		exit(EXIT_FAILURE);
    	}
}

/* scheduler */
static void schedule(int signum){
  // SET MODE BIT IF IT IS OS RETURN 


  // CHECK THE ALARM
		ALARM RUNTIME IS UP
		RUNTIME IS YIELDED

 
What to do when yielded or interrupted
	When a thread YIELDED and its not in running state we either 
		need to bring it to the running queue from ready queue READY
		clean it up if its TERMINATED - unblock and clean up blocked queue
		MOVE it to BLOCKING QUEUE when BLOCKED
	When RUNNING 
		Re-evaluate the prioritys
		run next in queue move other to new position in queue


	//RESETTIMER
	//RESET MODE TO USER
	//SWAP BACK TO OLD CONTEXT


SCHDULER_QUEUES

	//READY QUEUE // Threads wait here to be moved to the priority queue and given the running status
		When a thread yields or we are interrupted after some time. Check the ready queue and move the things there to the schedule queue based on method

	//RUNNING QUEUE// How things go on to it
		RR - Add to end of queue
		PSJF - Add to begining of queue 
		MLFQ - Add to begining of High Priority queue


	//RESOURCE QUEUE // Threads wait here for mutexs
		when a thread call lock but cant do anything move it to the resource queue (clean on unlock)
		if a thread has called unlock find the first if any that need it in this queue and move them to the run queue with ready priority

	//BLOCKING QUEUE // Threads wait here for joining 
		When a thread is calls join we change it to blocking state and move it to the blocking queue

	//CLEANING QUEUE // Threads wait here to clean up their stufff
		Go throught the cleaning queue and make sure that all locks are unlocked
		unlock any locks
		check the blocking queue for any threads that are waiting for you
		if there are any threads running place them on back on the high priority queue and change state to RUNNING with priority HIGH
		after everything is unlocked and unblocked call free on the stack and resources
		get next thread ready to running

  // if(signum == SIGALRM // ){
  //   printf("timer went off!\n");
  // }
  
  // setupSwapContexts();

  // if(MTH->current != NULL){
  // 	if(mode == RR){
// 		sched_RR();
  // 	}

  // 	if(mode == PSJF){
  // 		sched_PSJF();
  // 	}

  // 	if(mode == MLQ){
// 		sched_MLFQ();
  // 	}

  // if(MTH->current == NULL){
  // 	scheduleNextThread();
  // }


  // printf("sig handler returning\n");
  // setcontext(&ctx_main);

}

/* Round Robin scheduling algorithm */
static void sched_RR()
{
	//States
	NEW:
		if the thread is in new state return back and finish creating
	READY:
		if the thread is in ready state add to end of queue run thread that was interrupted
	RUNNING:
		if the thread is in running state dont do anything just some clean up maybe and resume
	BLOCKED:
		if the thread is waiting move it to waiting queue
	YIELDED:
		if the thread is in yielding move to end of the queue
	TERMINATED
		if the thread is in TERMINATED move to cleanup queue to return and free resources
	
	//SWAPPING
	TIMEING:
		once the quanta of time runs out swap to next in queue and move this to end of queue


	return;
}

/* Preemptive PSJF (STCF) scheduling algorithm */
static void sched_PSJF()
{
	NEW:
		if the thread is in new state return back  abd finish creating
	READY:
		if the thread is in ready state add to front of queue run the new thread swap the interrupted thread out
	RUNNING:
		if the thread is in running state dont do anything just some clean up maybe and resume
	BLOCKED:
		if the thread is waiting move it to waiting queue
	YIELDED:
		if the thread is in yielding move to end of the queue
	TERMINATED
		if the thread is in TERMINATED move to cleanup queue to return and free resources

	//SWAPPING
	Only swap when current thread is YIELDING or is TERMINATING

	return;
}

/* Preemptive MLFQ scheduling algorithm */
/* Graduate Students Only */
static void sched_MLFQ() {
	NEW:
		if the thread is in new state return back and finish creating
	READY:
		if the thread is in ready state add to high priority queue with value of HIGH priority new thread swap the interrupted thread out
	RUNNING:
		if the thread is in running state get ready to swap anything just some clean up maybe and resume
	BLOCKED:
		if the thread is waiting move it to waiting queue
	YIELDED:
		if the thread is in yielding move to end of the queue
	TERMINATED
		if the thread is in TERMINATED move to cleanup queue to return and free resources

	//Swapping
	TIMEING:
	run the next process in the HIGH queue

	RE_EVALUATING:
		When interrupting the running thread due to timer. Reevaluate 

	return;
}





// Feel free to add any other functions you need

// YOUR CODE HERE
