// File:	mypthread.c

// List all group members' names:
// iLab machine tested on:

#include "mypthread.h"
#include "Queue.c"

void setTimer(long int time_milli);

int mypthread_yield();

void disableTimer();

void printCurrSchedulerState();
void sched_RR();
void sched_PSJF();
void sched_MLFQ();

void (*current_scheduler)(void) = sched_PSJF;


void updatePriority(tcb_node* node, int change);

// Global Vars
unsigned int s_tid = 0;  // scheduler id
TH *MTH = NULL;          // scheduler
MH *mutexHandler = NULL; // mutex q
ucontext_t ctx_main;     // main context
ucontext_t ctx_handler;  // context handler
mode schedMode = RR;     // Which scheduler we use
int isSchedulerNotStarted = 1;
int isMutexQNotAllocated = 1;
struct sigaction act_timer;
sigset_t old_set, new_set;
struct itimerval timer;
int mode_bit = 0;
int load_balance = 21;
atomic_flag lock = ATOMIC_FLAG_INIT;
uint id_start = 2;
_Bool lockOld;

void setScheduler(mode scheduler_mode){
    switch (scheduler_mode){
        case RR:
            printf("\nUsing RR Scheduler\n");
            current_scheduler = sched_RR;
            break;
        case PSJF:
            printf("\nUsing PSJF Scheduler\n");
            current_scheduler = sched_PSJF;
            break;
        case MLFQ:
            printf("\nUsing MLFQ Scheduler\n");
            current_scheduler = sched_MLFQ;
            break;
        default:
            printf("\nUsing Default PSJF Scheduler\n");
            current_scheduler = sched_PSJF;
            break;
    }
}

void *executeThread()
{
    void *val = MTH->current->tcb->func_ptr(MTH->current->tcb->arg);
    lockOld = atomic_flag_test_and_set(&lock);
    if (!lockOld) {
        disableTimer();
        atomic_flag_clear(&lock);
        mypthread_exit(val);
    }
    else
    {
        perror("scheduler is unable to aquire lock for execution");
        exit(1);
    }
}
void calcAndAddExecTime(tcb* tcb){
    //  printf("\nTiming: Current Thread = %u\n", MTH->current->tcb->tid);
    clock_t end = clock(); //Get Time
    double execTime = (double)(end - tcb->start_exec) / (CLOCKS_PER_SEC); //Get Time per sec divide by 1000 to get ms
    tcb->total_exec = tcb->total_exec + (execTime * 1000); //Append time
    // printf("EXEC TIME = %f\n",tcb->total_exec);

}

void resetExecTime(tcb* tcb){
    tcb->total_exec = 0.0; //Append time
}

static void schedule(int signum)
{
    // if(MTH->current->tcb)
   lockOld = atomic_flag_test_and_set(&lock);
    if (!lockOld)
    {
        disableTimer();
        calcAndAddExecTime(MTH->current->tcb);
        //printf("\nScheduling: Current Thread = %d\n", MTH->current->tcb->tid);
        //printf("Entering Schedule Event Handler %d\n", signum);

        if (signum == SIGALRM)  
        {
            load_balance--;
            //printf("Your ran too long %u \n", MTH->current->tcb->tid);
            if(MTH->current->tcb->tid == 0){
                return;
            }
            // if(MTH->current->tcb->total_exec > MEDIUM_EXEC_TIMEOUT){
            //     printf("\nHigh Exec Thread Detected");
            // }
            if(MTH->current->tcb->priority = HIGH)
                enqueue(MTH->current, MTH->ready);
            else if(MTH->current->tcb->priority = MED)                
                enqueue(MTH->current, MTH->medium);
            else if(MTH->current->tcb->priority = LOW)
                enqueue(MTH->current, MTH->low);

            // //printQueue(MTH->ready);
        } else if (signum == MUTEX_HOLD)
                {
                    load_balance--;
            // I dont think we need to do anything
                }
        //        If a mutex is unlocked then find the
        else if (signum == YIELDED)
        {
            load_balance--;
            // printf("I am Yielding %u \n", MTH->current->tcb->tid);
            resetExecTime(MTH->current->tcb);
            // MTH->current->tcb->priority = HIGH;
            enqueue(MTH->current, MTH->ready);
        }
        else if (signum == BLOCKED)
        {
            load_balance--;
            resetExecTime(MTH->current->tcb);
            // printf("Blocking Thread %u \n",MTH->current->tcb->tid);
            enqueue(MTH->current, MTH->blocked);
            //printQueue(MTH->blocked);
        }
        else if (signum == TERMINATED)
        {
            load_balance--;
            // printf("exiting thread %u \n",MTH->current->tcb->tid);
            enqueue(MTH->current, MTH->terminated);
            //printQueue(MTH->terminated);
        }


        tcb_node *next_thread = NULL;
        tcb_node *prev_thread = MTH->current;
        //printCurrSchedulerState();
        if (isEmpty(MTH->running) || load_balance <= 0)
        {
            load_balance = 10;
            current_scheduler();
            //printf("out of shceduler \n");
            if(isEmpty(MTH->running) && isEmpty(MTH->medium) && isEmpty(MTH->low)){
                // printf("No More threads to run \n");
                //printCurrSchedulerState();
                exit(0);
            }
            
        }
        if(peek(MTH->running) != NULL){
            // printf("\nTaking from Running");
            next_thread = dequeue(MTH->running);
        }
        else if(peek(MTH->medium) != NULL){
            // printf("\nTaking from Medium");
            next_thread = dequeue(MTH->medium);
        }
        else if(peek(MTH->low) != NULL){
            // printf("\nTaking from Low");
            next_thread = dequeue(MTH->low);
        }
        else{
            printf("\nThere were no threads in any queue");
            exit(0);
        }
        
 

        ////printf("Next up is %u \n", next_thread->tcb->tid);
        next_thread->tcb->start_exec = clock();
        MTH->current = next_thread;

        ////printf("Swapping Context from %d\n", next_thread->tcb->tid);
        //printf("Leaving Schedule Event Handler\n");
        atomic_flag_clear(&lock);
        //printf("setting timer\n");  
        setTimer(HIGH_EXEC_TIMEOUT); 
        swapcontext(prev_thread->tcb->t_context, MTH->current->tcb->t_context);
        
    } else {
    //printf("oops interrupted the scheduler\n");
    return;
}
}

void disableTimer()
{
    if (sigprocmask(SIG_BLOCK, &act_timer.sa_mask, &act_timer.sa_mask) == -1) // SIGNAL is blocked
    {
        perror("sigprocmask");
    }

    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1)
    {
        perror("Unable to disable time in for MTH\n");
        exit(1);
    }
}

void setHandler()
{
    act_timer.sa_flags = 0;
    act_timer.sa_handler = &schedule;
    sigemptyset(&act_timer.sa_mask);
    sigaddset(&act_timer.sa_mask, SIGALRM);
    sigaction(SIGALRM, &act_timer, NULL);
}

void setTimer(long int time_milli)
{
    if (sigprocmask(SIG_UNBLOCK, &act_timer.sa_mask, &act_timer.sa_mask) == -1) // SIGNAL is unblocked
    {
        perror("sigprocmask");
    }
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = time_milli;
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1)
    {
        perror("Unable to set time in for MTH\n");
        exit(1);
    }
}


int startScheduler()
{
    if (MTH == NULL)
    {
        MTH = (TH *)malloc(sizeof(TH));
        initializeTH(MTH); // Setup the MTH's queues
        setHandler();
        //printf("Setting up main thread \n");
        tcb *main_thread = setupThread(&ctx_main, 0);
        MTH->current = createTCBNode(main_thread);
        //printf("main_thread created with id %d\n", main_thread->tid);
        //printQueue(MTH->ready);
        isSchedulerNotStarted = 0;
        return 1;
    }
    return -1;
}

int mypthread_create(mypthread_t *thread, pthread_attr_t *attr, void *(*function)(void *), void *arg)
{
    lockOld = atomic_flag_test_and_set(&lock);
    if (!lockOld)
    {
        disableTimer();
        if (isSchedulerNotStarted)
        {
            //printf("Setting Up\n");
            startScheduler();
            //printf("Done Setting Up\n");
            isSchedulerNotStarted = 0;
            getcontext(&ctx_main);
        }

        ucontext_t *thread_context = (ucontext_t *)malloc(sizeof(ucontext_t));
        getcontext(thread_context);

        if ((thread_context->uc_stack.ss_sp = (char *)malloc(T_STACK_SIZE)) != NULL)
        {
            thread_context->uc_link = 0 ;
            thread_context->uc_stack.ss_size = T_STACK_SIZE;
            thread_context->uc_stack.ss_flags = 0;

            makecontext(thread_context, (void *)&executeThread, 0);

            tcb *new_thread = setupThread(thread_context, -1);
            new_thread->func_ptr = function;
            new_thread->arg = arg;
            new_thread->priority = HIGH;
            enqueue(createTCBNode(new_thread), MTH->ready);

            *thread = new_thread->tid;
            //printf("Making Thread with Id %u\n",new_thread->tid);
            atomic_flag_clear(&lock);
            setTimer(HIGH_EXEC_TIMEOUT);
            return new_thread->tid;
        }
        else
        {
            //printf("Cannot Malloc for stack space");
            exit(1);
        }
    }
    return -1;
};

int mypthread_yield()
{
    lockOld = atomic_flag_test_and_set(&lock);
    if (!lockOld)
    {
        disableTimer();
        atomic_flag_clear(&lock);
        schedule(YIELDED);
    }
};

void mypthread_exit(void *value_ptr)
{
    lockOld = atomic_flag_test_and_set(&lock);
    if (!lockOld)
    {
        disableTimer();
        ////printf("Thread is exiting");
        MTH->current->tcb->t_retval = value_ptr;
        atomic_flag_clear(&lock);
        ////printf("Exiting %u\n ",MTH->current->tcb->tid);
        schedule(TERMINATED);
    }
    else
    {
        //printf("scheduler is unable to aquire lock for exit %d\n", mode_bit);
        exit(1);
    }
};

int mypthread_join(mypthread_t thread, void **value_ptr)
{
    lockOld = atomic_flag_test_and_set(&lock);
    if (!lockOld)
    {
        disableTimer();
        //printf("Thread is blocked");
        MTH->current->tcb->join_id = thread;
        atomic_flag_clear(&lock);
        schedule(BLOCKED);
        
        
    }
    else
    {
        //printf("scheduler is unable to aquire lock for join %d\n", mode_bit);
        exit(1);
    }
    lockOld = atomic_flag_test_and_set(&lock);
    if (!lockOld)
    {
       
        if (value_ptr != NULL)
        {
            *value_ptr = MTH->current->tcb->t_retval;
            }
        atomic_flag_clear(&lock);
        return 0;

    } else {
        //printf("scheduler is unable to aquire lock for join conclusion %d\n", mode_bit);
        exit(1);
    
    }
   return 0;
};

void printCurrSchedulerState(){
     //printf("current scheduler state { \n");
                    //printQueue(MTH->ready);
                    //printQueue(MTH->running);
                    //printQueue(MTH->blocked);
                    //printQueue(MTH->terminated);
                    //printf("}\n");
}

void handleBlockedQueue(){
    if (!isEmpty(MTH->blocked) && !isEmpty(MTH->terminated))
            {
                //printf("Checking blocked queue for joinable threads\n");
                tcb_node *temp_waiting = dequeue(MTH->blocked);
                mypthread_t id_to_find = temp_waiting->tcb->join_id;

                //printf("looking for %u\n", id_to_find);
                tcb_node *terminated_thread = searchQueueAndRemove(MTH->terminated, id_to_find);
                if (terminated_thread != NULL)
                {
                    temp_waiting->tcb->join_id = 0;



                    //printf("FOUND IT %u\n", id_to_find);
                    temp_waiting->tcb->t_retval = terminated_thread->tcb->t_retval;
                    if (terminated_thread->tcb->t_retval != NULL)
                    {
                        //printf("%u got value %d from thread %u\n",temp_waiting->tcb->tid, *(int*)temp_waiting->tcb->t_retval, terminated_thread->tcb->tid);
                    }
                    enqueue(temp_waiting, MTH->ready);
                    // deleteTerminatedNode;
                }
                else
                {
                    //printf("Couldnt find %u\n", id_to_find);
                    enqueue(temp_waiting, MTH->blocked);
                }
            }
}

void sched_MLFQ() {
            //printf("Scheduler Called\n");
            //printf("in scheduler \n");
            //printCurrSchedulerState(); //Prints State
            handleBlockedQueue(); //Checks Blocked Queue For Joins

            // handler high -> medium
            tcb_node *run_prev, *run_node = MTH->ready->front;
            int sort_counter = 0;
            while(run_node && sort_counter < MTH->ready->size){
                run_prev = run_node;
                if(run_node->tcb->total_exec >= MEDIUM_EXEC_TIMEOUT){
                    run_node->tcb->priority = MED;
                    enqueue(searchQueueAndRemove(MTH->ready, run_node->tcb->tid), MTH->medium);
                }
                if(run_prev != run_node){
                    run_node = run_prev->next;
                }
                else{
                    run_node = run_node->next;
                }
                sort_counter++;
            }

            tcb_node *med_prev, *med_node = MTH->medium->front;
            sort_counter = 0;
            while(med_node && sort_counter < MTH->medium->size){
                med_prev = med_node;
                if(med_node->tcb->total_exec >= LOW_EXEC_TIMEOUT){
                    med_node->tcb->priority = LOW;
                    enqueue(searchQueueAndRemove(MTH->medium, med_node->tcb->tid), MTH->low);
                }
                if(med_prev != med_node){
                    med_node = med_prev->next;
                }
                else{
                    med_node = med_node->next;
                }
                sort_counter++;
            }

            if (!isEmpty(MTH->ready))
            {
                //printf("transferQueue time baby\n");
                transferQueueSJF(MTH->ready, MTH->running);
                //printf("Done Transfering\n");
            }

            // No need for low as it will be FCFS


            //printf("Scheduler is done\n");
            //printCurrSchedulerState();

}



void sched_PSJF() {
            //printf("Scheduler Called\n");
            //printf("in scheduler \n");
            //printCurrSchedulerState(); //Prints State
            handleBlockedQueue(); //Checks Blocked Queue For Joins
            
            if (!isEmpty(MTH->ready))
            {
                //printf("transferQueue time baby\n");
                transferQueueSJF(MTH->ready, MTH->running);
                //printf("Done Transfering\n");
            }

            //printf("Scheduler is done\n");
            //printCurrSchedulerState();

}


void sched_RR()
{
            //printf("Scheduler Called\n");
            //printf("in scheduler \n");
            //printCurrSchedulerState(); //Prints State
            handleBlockedQueue(); //Checks Blocked Queue For Joins
            
            if (!isEmpty(MTH->ready))
            {
                //printf("transferQueue time baby\n");
                transferQueue(MTH->ready, MTH->running);
                //printf("Done Transfering\n");
            }
            //printf("Scheduler is done\n");
            //printCurrSchedulerState();
}

// Mutex code...................................................................

int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
    lockOld = atomic_flag_test_and_set(&lock);
    if (!lockOld) {
        mutex->flag = 0;
        mutex->guard = 0;
        mutex->hold_queue = createQueue("mutex queue");
        mutex->owner = NULL;
        atomic_flag_clear(&lock);
    }
    return 1;
}

/* Aquire a mutex (lock) */
int mypthread_mutex_lock(mypthread_mutex_t* mutex)
{
    lockOld = atomic_flag_test_and_set(&lock);
    if (!lockOld)
    {
        ////printf("Mutex lock\n");
        disableTimer();
        if(mutex == NULL){
            perror("Calling mutex that is not initialized\n");
            exit(0);
        }


        //If it is unlocked
        if(mutex->guard == 0) {

            mutex->guard = 1;
            mutex->owner = MTH->current->tcb;
            atomic_flag_clear(&lock);
            setTimer(HIGH_EXEC_TIMEOUT);
            return 0;
        }
        if (mutex->owner->tid == MTH->current->tcb->tid) {
                //printf("\nThread tried to double lock a mutex that it already owns");
                exit(1);
            }
         //   //printf("Mutex is locked going on queue %u\n", MTH->current->tcb->tid);
        ////printf("Printing Queue\n");
        //getchar();
        enqueue(MTH->current, mutex->hold_queue);
        //printQueue(mutex->hold_queue);
        atomic_flag_clear(&lock);
        schedule(MUTEX_HOLD);
        return 0;
    
    }
    else{
        perror("Failed to obtain lock for mutex lock check\n");
        exit(1);
    }
}

/* release a mutex (unlock) */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex)
{
    lockOld = atomic_flag_test_and_set(&lock);
    disableTimer();
    if(!lockOld){
        
        ////printf("\n Mutex unlock");
        if(mutex -> guard == 0){
            // atomic_flag_clear(&lock); add back
            // setTimer(HIGH_EXEC_TIMEOUT); add back
            perror("Called unlock on an unlocked mutex"); //REMOVE
            exit(1);//REMOVE
            //return 0; add back
        }

        if( mutex->owner->tid != MTH->current->tcb->tid) {
                //printf("%u thread called unlock, but lock belongs to %u\n", MTH->current->tcb->tid, mutex->owner->tid);
                ////printQueue(mutex->hold_queue);
                exit(0);
        }

        mutex->owner = NULL;
        mutex->guard = 0;
        if(!isEmpty(mutex->hold_queue)){ 
            // getchar();
            tcb_node *node = dequeue(mutex->hold_queue);
            mutex->owner = node->tcb;
            //printf("Giving lock ownership to %u\n",mutex->owner->tid);
            mutex->guard = 1;
            enqueue(node, MTH->ready);
            //printQueue(mutex->hold_queue);
            //printQueue(MTH->ready);
        }
        atomic_flag_clear(&lock);
        setTimer(HIGH_EXEC_TIMEOUT);
        return 1;
        

    }
        else{
       perror("Failed to obtain lock for mutex lock check\n");
        exit(1);
    }
    return 1;

}

/* destroy a mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex)
{
    lockOld = atomic_flag_test_and_set(&lock);
    if(!lockOld){
        disableTimer();
        mutex = NULL;
        atomic_flag_clear(&lock);
        setTimer(HIGH_EXEC_TIMEOUT);
        return 1;
    }
        else{
        perror("Unable to get lock to destroy mutexes");
        exit(1);
    }
}

void initializeTH(TH *scheduler)
{
    scheduler->current = NULL;
    scheduler->s_tcb_node = NULL;
    scheduler->running = createQueue("running");
    scheduler->medium = createQueue("medium");
    scheduler->low = createQueue("low");
    scheduler->ready = createQueue("ready");
    scheduler->terminated = createQueue("terminated");
    scheduler->blocked = createQueue("blocked");
    scheduler->resource = createQueue("resource");
}

void getTime(struct timespec time)
{
    clock_gettime(CLOCK_REALTIME, &time);
}

tcb *setupThread(ucontext_t *context, mypthread_t join_id)
{
    tcb *t = (tcb *)malloc(sizeof(tcb));
    t->tid = id_start++;
    t->priority = HIGH;
    t->name = NULL;
    t->t_retval = NULL;
    t->t_context = context;
    t->start_exec = clock();
    t->total_exec = 0;
    t->join_id = join_id; // parent thread
    return t;
}

void resetPriority(tcb_node* node){
    if(node == NULL){
        perror("Trying to change priority on null node");
        return;
    }
    node->tcb->priority = LOW;
}

void updatePriority(tcb_node* node, int change){
    if(node == NULL){
        perror("Trying to change priority on null node");
        return;
    }
    if(change != 1 ||change != -1){
        perror("Trying to change priority by a value greater than one dont use this method");
        return;
    }

    t_priority currPri = node->tcb->priority;
    switch(currPri){
        LOW:
            if(change == 1)
                node->tcb->priority = MED;

            if(change == -1)
                node->tcb->priority = LOW;
            break;
        MED:
            if(change == 1)
                node->tcb->priority = HIGH;

            if(change == -1)
                node->tcb->priority = LOW;
            break;
        HIGH:
            if(change == 1)
                node->tcb->priority = HIGH;

            if(change == -1)
                node->tcb->priority = MED;
            break;
    }
}