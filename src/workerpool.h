/*****************************************************************************
 * Package workerpool is used to create and manage a threadpool.
 */
#ifndef WORKERPOOL_H
#define WORKERPOOL_H

#include <stdbool.h>
#include <stddef.h>

/*****************************************************************************
 * tpool_t is the main structure for the workpool of threads
 */
typedef struct tpool tpool_t;

/*****************************************************************************
 * thread_func_t
 */
typedef void (*thread_func_t)(void *arg);

/*****************************************************************************
 * function prototypes
 */
tpool_t *tpool_create(size_t num);
void tpool_destroy(tpool_t *tm);
bool tpool_add_work(tpool_t *tm, thread_func_t func, void *arg);
void tpool_wait(tpool_t *tm);

#endif