// File:	mypthread.c

// List all group members' names:
// iLab machine tested on:

#include "mypthread.h"
#include "Queue.c"

void setTimer(long int time_milli);

int mypthread_yield();

void sched_RR();

void disableTimer();

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
int load_balance = 0;
atomic_flag lock = ATOMIC_FLAG_INIT;

void *executeThread()
{
    tcb *current_thread = MTH->current->tcb;
    // printf("Running Thread %u\n", current_thread->tid);
    void *val = current_thread->func_ptr(current_thread->arg);
    if (__sync_add_and_fetch(&mode_bit, 1) == 1)
    {
        disableTimer();
        if (__sync_add_and_fetch(&mode_bit, -1) == 0)
        {
            mypthread_exit(val);
        }
        else
        {
            // printf("scheduler is unable to release lock for execution");
            exit(1);
        }
    }
    else
    {
        // printf("scheduler is unable to aquire lock for execution");
        exit(1);
    }
}

static void schedule(int signum)
{
    // if(MTH->current->tcb)
    // printf("\nScheduling: Current Thread = %d", MTH->current->tcb->tid);
    setTimer(HIGH_EXEC_TIMEOUT);
    if (__sync_add_and_fetch(&mode_bit, 1) == 1)
    {
        disableTimer();
        // printf("Entering Schedule Event Handler %d\n", signum);

        if (signum == SIGALRM)
        {
            // printf("Your ran too long %u \n", MTH->current->tcb->tid);
            enqueue(MTH->current, MTH->ready);
            // printQueue(MTH->ready);
        }
        if (signum == MUTEX_HOLD)
        {
            // I dont think we need to do anything
        }
        //        If a mutex is unlocked then find the
        if (signum == YIELDED)
        {
            // printf("I am Yielding %u \n", MTH->current->tcb->tid);
            if (load_balance == 0)
            {
                enqueue(MTH->current, MTH->ready);
            }
            load_balance = 0;
        }
        if (signum == BLOCKED)
        {
            // printf("exiting thread %u \n",MTH->current->tcb->tid);
            enqueue(MTH->current, MTH->blocked);
            // printQueue(MTH->blocked);
        }
        if (signum == TERMINATED)
        {
            // printf("exiting thread %u \n",MTH->current->tcb->tid);
            enqueue(MTH->current, MTH->terminated);
            // printQueue(MTH->terminated);
        }

        // printf("current scheduler state { \n");
        //  printQueue(MTH->ready);
        //  printQueue(MTH->running);
        //  printQueue(MTH->blocked);
        //  printQueue(MTH->terminated);
        // printf("}\n");

        // getchar();
        tcb_node *prev_thread = MTH->current;
        tcb_node *next_thread = dequeue(MTH->running);
        if (next_thread == NULL)
        {
            // printf("Running Queue is Empty going to scheduler \n");
            next_thread = MTH->s_tcb_node;
            load_balance = 1;
        }
        // printf("Next up is %u \n", next_thread->tcb->tid);
        MTH->current = next_thread;

        if (__sync_add_and_fetch(&mode_bit, -1) == 0)
        {

            // printf("Leaving Schedule Event Handler\n");
            // printf("Swapping Context to %u\n ",MTH->current->tcb->tid);
            if (MTH->current->tcb != 0)
            {
                setTimer(HIGH_EXEC_TIMEOUT);
            }
            swapcontext(prev_thread->tcb->t_context, MTH->current->tcb->t_context);
        }
        else
        {
            // printf("Unable to leave Schedule Event Handler %d\n", mode_bit);
            exit(1);
        }
        return;
    }
    // printf("oops interrupted the scheduler\n");
    return;
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

void setupSchedulerContext()
{
    getcontext(&ctx_handler);
    ctx_handler.uc_link = 0;
    ctx_handler.uc_stack.ss_sp = malloc(T_STACK_SIZE);
    ctx_handler.uc_stack.ss_size = T_STACK_SIZE;
    ctx_handler.uc_stack.ss_flags = 0;
    return makecontext(&ctx_handler, (void *)&sched_RR, 0);
}

int startScheduler()
{
    if (MTH == NULL)
    {

        MTH = (TH *)malloc(sizeof(TH));
        initializeTH(MTH); // Setup the MTH's queues
        setHandler();
        setupSchedulerContext(); // setup scheduler context
        tcb *s_tcb = setupThread(&ctx_handler, -1);
        s_tcb->tid = 0;
        s_tcb->t_context = &ctx_handler;
        MTH->s_tcb_node = createTCBNode(s_tcb);
        // printf("scheduler_thread created with id %u\n", s_tcb->tid);
        // printf("Setting up main thread \n");
        tcb *main_thread = setupThread(&ctx_main, s_tcb->tid);
        tcb_node *main_node = createTCBNode(main_thread);
        // printf("main_thread created with id %d\n", main_thread->tid);
        // enqueue(main_node, MTH->ready);
        enqueue(MTH->s_tcb_node, MTH->ready);
        // printf("First Ready Queue : ");
        // printQueue(MTH->ready);
        isSchedulerNotStarted = 0;
        MTH->current = main_node;
        return 1;
    }
    return -1;
}

int mypthread_create(mypthread_t *thread, pthread_attr_t *attr, void *(*function)(void *), void *arg)
{
    if (__sync_add_and_fetch(&mode_bit, 1) == 1)
    {
        disableTimer();
        if (isSchedulerNotStarted)
        {
            // printf("Setting Up\n");
            startScheduler();
            // printf("Done Setting Up\n");
            isSchedulerNotStarted = 0;
        }

        ucontext_t *thread_context = (ucontext_t *)malloc(sizeof(ucontext_t));
        getcontext(thread_context);

        if ((thread_context->uc_stack.ss_sp = (char *)malloc(T_STACK_SIZE)) != NULL)
        {
            thread_context->uc_link = 0;
            thread_context->uc_stack.ss_size = T_STACK_SIZE;
            thread_context->uc_stack.ss_flags = 0;

            makecontext(thread_context, (void *)&executeThread, 0);

            tcb *new_thread = setupThread(thread_context, -1);
            new_thread->func_ptr = function;
            new_thread->arg = arg;
            enqueue(createTCBNode(new_thread), MTH->ready);

            *thread = new_thread->tid;
            // printf("Making Thread with Id %u\n",new_thread->tid);
            if (__sync_add_and_fetch(&mode_bit, -1) == 0)
            {
                setTimer(HIGH_EXEC_TIMEOUT);
                return new_thread->tid;
            }
            else
            {
                // printf("Unable to leave thread creation %d\n", mode_bit);
                exit(1);
            }
        }
        else
        {
            // printf("Cannot Malloc for stack space");
            exit(1);
        }
    }
    return -1;
};

int mypthread_yield()
{
    if (__sync_add_and_fetch(&mode_bit, 1) == 1)
    {
        disableTimer();
        if (__sync_add_and_fetch(&mode_bit, -1) == 0)
        {
            schedule(YIELDED);
        }
    }
};

void mypthread_exit(void *value_ptr)
{
    if (__sync_add_and_fetch(&mode_bit, 1) == 1)
    {
        disableTimer();
        // printf("Thread is exiting");
        MTH->current->tcb->t_retval = value_ptr;
        if (__sync_add_and_fetch(&mode_bit, -1) == 0)
        {
            schedule(TERMINATED);
        }
        else
        {
            // printf("scheduler is unable to release lock for exit %d\n", mode_bit);
            exit(1);
        }
    }
    else
    {
        // printf("scheduler is unable to aquire lock for exit %d\n", mode_bit);
        exit(1);
    }
};

int mypthread_join(mypthread_t thread, void **value_ptr)
{
    if (__sync_add_and_fetch(&mode_bit, 1) == 1)
    {
        disableTimer();
        // printf("Thread is blockeding");
        if (__sync_add_and_fetch(&mode_bit, -1) == 0)
        {
            MTH->current->tcb->join_id = thread;
            schedule(BLOCKED);
        }
        else
        {
            // printf("scheduler is unable to release lock for join %d\n", mode_bit);
            exit(1);
        }
    }
    else
    {
        // printf("scheduler is unable to aquire lock for join %d\n", mode_bit);
        exit(1);
    }
    if (value_ptr != NULL)
    {
        *value_ptr = MTH->current->tcb->t_retval;
    }
    return 0;
};

void sched_RR()
{
    while (1)
    {
        // printf("Scheduler Called");
        if (__sync_add_and_fetch(&mode_bit, 1) == 1)
        {
            // printf("in scheduler \n");
            disableTimer();
            // getchar();

            if (!isEmpty(MTH->ready))
            {
                // printQueue(MTH->ready);
                transferQueue(MTH->ready, MTH->running);
                // printQueue(MTH->running);
                // printQueue(MTH->ready);
            }
            if (!isEmpty(MTH->blocked) && !isEmpty(MTH->terminated))
            {
                // printf("Checking blocked queue for joinable threads\n");
                tcb_node *temp_waiting = dequeue(MTH->blocked);
                mypthread_t id_to_find = temp_waiting->tcb->join_id;

                // printf("looking for %u\n", id_to_find);
                tcb_node *terminated_thread = searchQueueAndRemove(MTH->terminated, id_to_find);
                if (terminated_thread != NULL)
                {
                    // printf("FOUND IT%u\n", id_to_find);
                    temp_waiting->tcb->t_retval = terminated_thread->tcb->t_retval;
                    if (terminated_thread->tcb->t_retval != NULL)
                    {
                        // printf("%u got value %d from thread %u\n",temp_waiting->tcb->tid, *(int*)temp_waiting->tcb->t_retval, terminated_thread->tcb->tid);
                    }
                    enqueue(temp_waiting, MTH->ready);
                    // deleteTerminatedNode;
                }
                else
                {
                    enqueue(temp_waiting, MTH->blocked);
                }
            }

            // getchar();
            if (__sync_add_and_fetch(&mode_bit, -1) == 0)
            {
                mypthread_yield();
            }
            else
            {
                // printf("scheduler is unable to release lock %d\n", mode_bit);
                exit(1);
            }
        }
        else
        {
            // printf("scheduler is unable to aquire lock %d\n", mode_bit);
            exit(1);
        }
    };
}

int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
    // return 0;
    mutex->flag = 0;
    mutex->guard = 0;
    mutex->hold_queue = createQueue("mutex queue");
    mutex->owner = NULL;

    // Set OS BIT
    if (isMutexQNotAllocated)
    {
        mutexHandler = (MH *)malloc(sizeof(MH));
        isMutexQNotAllocated = 0;
    }
    // If the same mutex is being tried to create again
    mutex_node *mutexListCurr = mutexHandler->mutexList;
    while (mutexListCurr)
    {
        if (mutexListCurr->mutex == mutex)
        {
            //            printf("Mutex reinstantiated");
            exit(1);
        }
        mutexListCurr = mutexListCurr->next;
    }

    if (mutexHandler->mutex_size == 0)
    {
        mutexHandler->mutexList = (mutex_node *)malloc(sizeof(mutex_node));
        mutexHandler->mutex_size = 1;
        mutexHandler->mutexList->mutex = mutex;
        mutexHandler->mutexList->next = NULL;
        return 1;
    }

    mutex_node *newMutexNode = (mutex_node *)malloc(sizeof(mutex_node *));
    mutex_node *curr = mutexHandler->mutexList;
    while (curr->next != NULL)
    {
        curr = curr->next;
    }
    newMutexNode->mutex = mutex;
    newMutexNode->next = NULL;
    curr->next = newMutexNode;
    mutexHandler->mutex_size++;
    return 1;
}

/* Aquire a mutex (lock) */
int mypthread_mutex_lock(mypthread_mutex_t *mutex)
{
    while (1)
    {
        if (!atomic_flag_test_and_set(&lock))
        {
            disableTimer();
            // printf("\nytex Lock");
            mypthread_t current_tid = MTH->current->tcb->tid;
            int isOnHold = 0;
            tcb_node *searchedNode = NULL;
            if (mutex->hold_queue)
                searchQueue(mutex->hold_queue, current_tid);
            if (searchedNode)
            {
                isOnHold = searchedNode->tcb->tid == current_tid;
            }
            if (mutex->guard == 0 && !isOnHold)
            {
                mutex->guard = 1;
                mutex->owner = MTH->current->tcb;
                atomic_flag_clear(&lock);
                setTimer(HIGH_EXEC_TIMEOUT);
                return 1;
            }
            if (mutex->guard == 0 && isOnHold && peek(mutex->hold_queue)->tid == current_tid)
            {
                tcb_node *firstWaitingThreadNode = dequeue(mutex->hold_queue);
                // printf("\nLocking");
                mutex->guard = 1;
                mutex->owner = firstWaitingThreadNode->tcb;
                free(firstWaitingThreadNode);
                // Scheduele
                atomic_flag_clear(&lock);
                setTimer(HIGH_EXEC_TIMEOUT);
                return 1;
            }

            if (mutex->guard == 1 && mutex->owner->tid == current_tid)
            {
                printf("\nThread tried to double lock a mutex that it already owns");
                // exit(1);
                return 1;
            }

            if (mutex->hold_queue)
            {
                if (mutex->guard == 1 && searchQueue(mutex->hold_queue, current_tid) != NULL)
                {
                    printf("\nThread tried to lock a mutex that was previously requested to lock");
                    // exit(1);
                    return 1;
                }
            }
            else
            {
                atomic_flag_clear(&lock);
                setTimer(HIGH_EXEC_TIMEOUT);
                return 0;
            }

            enqueue(createTCBNode(MTH->current->tcb), mutex->hold_queue);
            schedule(MUTEX_HOLD);
            setTimer(HIGH_EXEC_TIMEOUT);
            atomic_flag_clear(&lock);
            return 1;
        }
        else
        {
            schedule(YIELDED);
        }
    }
    return 1;
}

/* release a mutex (unlock) */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex)
{
    while (1)
    {
        // return 0;
        if (!atomic_flag_test_and_set(&lock))
        {
            disableTimer();
            // printf("\n Mutex unlock");
            mypthread_t current_tid = MTH->current->tcb->tid;
            if (mutex->guard == 1 && mutex->owner->tid == current_tid)
            {
                mutex->owner = NULL;
                // printf("\nUnlocking");
                mutex->guard = 0;
                if (mutex->hold_queue)
                    if (peek(mutex->hold_queue) != NULL)
                    {
                        tcb_node *node = dequeue(mutex->hold_queue);
                        enqueue(node, MTH->ready);
                    }
                setTimer(HIGH_EXEC_TIMEOUT);
            }
            atomic_flag_clear(&lock);
            return 1;
        }
        else
        {
            schedule(YIELDED);
        }
    }
}

/* destroy a mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex)
{
    // return 0;
    mutex_node *curr = mutexHandler->mutexList, *prev;
    if (curr->next == NULL)
    {
        free(mutexHandler->mutexList);
        mutexHandler->mutex_size--;
        return 1;
    }
    while (curr->next != NULL)
    {
        prev = curr;
        if (curr->mutex == mutex)
            break;

        curr = curr->next;
    }

    prev->next = curr->next;
    mutexHandler->mutex_size--;
    return 1;
}

void initializeTH(TH *scheduler)
{
    scheduler->running = createQueue("running");
    scheduler->medium = createQueue("medium");
    scheduler->low = createQueue("low");
    scheduler->ready = createQueue("ready");
    scheduler->terminated = createQueue("terminated");
    scheduler->blocked = createQueue("mutex_locked");
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
    t->tid = rand();
    t->priority = HIGH;
    t->name = NULL;
    t->t_retval = NULL;
    t->t_context = context;
    t->state = RUNNING;
    t->join_id = join_id; // parent thread
    return t;
}