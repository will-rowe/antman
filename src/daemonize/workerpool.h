#ifndef WORKER_H
#define WORKER_H

#include <stdbool.h>
#include <stddef.h>

//
typedef struct tpool tpool_t;
typedef void (*thread_func_t)(void *arg);

/*
    function declarations
*/
tpool_t* tpool_create(size_t num);
void tpool_destroy(tpool_t* tm);
bool tpool_add_work(tpool_t* tm, thread_func_t func, void* arg);
void tpool_wait(tpool_t* tm);

#endif