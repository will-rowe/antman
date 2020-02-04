#include <pthread.h>
#include <stdlib.h>

#include "3rd-party/slog.h"

#include "workerpool.h"

/*
    the worker pool is a simple linked list which stores the function to call and its arguments
*/

// tpool_work
typedef struct tpool_work
{
    thread_func_t func;
    void *arg;
    struct tpool_work *next;
} tpool_work_t;

// tpool
struct tpool
{
    tpool_work_t *work_first;    // pop objects
    tpool_work_t *work_last;     // push objects
    pthread_mutex_t work_mutex;  // thread lock
    pthread_cond_t work_cond;    // signals the threads that there is work to be processed
    pthread_cond_t working_cond; // signals when there are no threads processing
    size_t working_cnt;          // how many threads are actively processing work
    size_t thread_cnt;           // helps us prevent running threads from being destroyed prematurely
    bool stop;                   // used to stop the threads
};

// tpool_work_create is used to create a work object
static tpool_work_t *tpool_work_create(thread_func_t func, void *arg)
{
    tpool_work_t *work;
    if (func == NULL)
        return NULL;
    work = malloc(sizeof(*work));
    work->func = func;
    work->arg = arg;
    work->next = NULL;
    return work;
}

// tpool_work_destroy is used to destroy a work object
static void tpool_work_destroy(tpool_work_t *work)
{
    if (work == NULL)
        return;
    free(work);
}

// tpool_work_get pulls work off the queue and maintains the linked list
static tpool_work_t *tpool_work_get(tpool_t *tp)
{
    tpool_work_t *work;

    if (tp == NULL)
        return NULL;

    work = tp->work_first;
    if (work == NULL)
        return NULL;

    if (work->next == NULL)
    {
        tp->work_first = NULL;
        tp->work_last = NULL;
    }
    else
    {
        tp->work_first = work->next;
    }

    return work;
}

// tpool_worker waits for work and then processes it
static void *tpool_worker(void *arg)
{
    tpool_t *tp = arg;
    tpool_work_t *work;

    // keep the thread running
    while (1)
    {

        // mutex lock is used to synchronize pulling work from the queue
        pthread_mutex_lock(&(tp->work_mutex));

        // check we don't need to stop before pulling any work off the queue
        if (tp->stop)
            break;

        // check if there is any work available for processing and wait in a conditional if there is none
        if (tp->work_first == NULL)
            pthread_cond_wait(&(tp->work_cond), &(tp->work_mutex));

        // once the thread was signaled there is work, get it
        work = tpool_work_get(tp);

        // notify the pool that this thread is working
        tp->working_cnt++;

        // unlock the mutex so other threads can get work from the queue
        pthread_mutex_unlock(&(tp->work_mutex));

        /*
            If there was work, process it and destroy the work object.
            It is possible that there was no work at this point so there
            isn’t anything that needs to be done.
            
            For example, lets say there is one piece of work and 4 threads.
            All threads are signaled there is work.
            Each one will unblock one at a time and pull the work, so the 
            first thread will acquire the lock pull the work, lock and start processing. 
            The next three will unblock and pull nothing from the queue because it’s empty.
        */
        if (work != NULL)
        {
            work->func(work->arg);
            tpool_work_destroy(work);
        }

        // lock the mutex again and clean up the thread
        pthread_mutex_lock(&(tp->work_mutex));
        tp->working_cnt--; // notify the pool that this thread is no longer working
        if (!tp->stop && tp->working_cnt == 0 && tp->work_first == NULL)
            pthread_cond_signal(&(tp->working_cond));
        pthread_mutex_unlock(&(tp->work_mutex));
    }

    tp->thread_cnt--;
    pthread_cond_signal(&(tp->working_cond));
    pthread_mutex_unlock(&(tp->work_mutex));
    return NULL;
}

// tpool_create is used to create a workerpool with a specified number of workers (2 is used if num < 2)
tpool_t *tpool_create(size_t num)
{
    tpool_t *tp;
    pthread_t thread;
    size_t i;

    if (num < 2)
        num = 2;

    tp = calloc(1, sizeof(*tp));
    tp->thread_cnt = num;

    pthread_mutex_init(&(tp->work_mutex), NULL);
    pthread_cond_init(&(tp->work_cond), NULL);
    pthread_cond_init(&(tp->working_cond), NULL);

    tp->work_first = NULL;
    tp->work_last = NULL;

    for (i = 0; i < num; i++)
    {
        pthread_create(&thread, NULL, tpool_worker, tp);
        pthread_detach(thread);
    }

    return tp;
}

// tpool_destroy
void tpool_destroy(tpool_t *tp)
{
    tpool_work_t *work;
    tpool_work_t *work2;

    if (tp == NULL)
        return;

    pthread_mutex_lock(&(tp->work_mutex));
    work = tp->work_first;
    while (work != NULL)
    {
        work2 = work->next;
        tpool_work_destroy(work);
        work = work2;
    }
    tp->stop = true;
    pthread_cond_broadcast(&(tp->work_cond));
    pthread_mutex_unlock(&(tp->work_mutex));

    tpool_wait(tp);

    pthread_mutex_destroy(&(tp->work_mutex));
    pthread_cond_destroy(&(tp->work_cond));
    pthread_cond_destroy(&(tp->working_cond));

    free(tp);
}

// tpool_add_work
bool tpool_add_work(tpool_t *tp, thread_func_t func, void *arg)
{
    tpool_work_t *work;
    if (tp == NULL)
        return false;
    work = tpool_work_create(func, arg);
    if (work == NULL)
        return false;
    pthread_mutex_lock(&(tp->work_mutex));
    if (tp->work_first == NULL)
    {
        tp->work_first = work;
        tp->work_last = tp->work_first;
    }
    else
    {
        tp->work_last->next = work;
        tp->work_last = work;
    }
    pthread_cond_broadcast(&(tp->work_cond));
    pthread_mutex_unlock(&(tp->work_mutex));
    return true;
}

// tpool_wait
void tpool_wait(tpool_t *tp)
{
    if (tp == NULL)
        return;

    pthread_mutex_lock(&(tp->work_mutex));
    while (1)
    {
        if ((!tp->stop && tp->working_cnt != 0) || (tp->stop && tp->thread_cnt != 0))
        {
            pthread_cond_wait(&(tp->working_cond), &(tp->work_mutex));
        }
        else
        {
            break;
        }
    }
    pthread_mutex_unlock(&(tp->work_mutex));
}