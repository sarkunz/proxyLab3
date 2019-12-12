#include "csapp.h"
#include "logbuf.h"

void logbuf_init(logbuf_t *sp, int n)
{
    sp->buf = calloc(n, sizeof(char*)); 
    sp->n = n;                       /* Buffer holds max of n items */
    sp->front = sp->rear = 0;        /* Empty buffer iff front == rear */
    sem_init(&sp->mutex, 0, 1);      /* Binary semaphore for locking */
    sem_init(&sp->slots, 0, n);      /* Initially, buf has n empty slots */
    sem_init(&sp->items, 0, 0);      /* Initially, buf has zero data items */
}

void logbuf_deinit(logbuf_t *sp, int n)
{
    for (int i = 0; i < n; i++) {
        free(sp->buf[i]);
    }
    free(sp->buf);
}

void logbuf_insert(logbuf_t *sp, char* item)
{
    if (sem_wait(&sp->slots) < 0) printf("sem_wait error");                          /* Wait for available slot */
    if (sem_wait(&sp->mutex) < 0) printf("sem_wait error");                           /* Lock the buffer */
    sp->buf[(++sp->rear)%(sp->n)] = item;   /* Insert the item */
    if (sem_post(&sp->mutex) < 0) printf("sem_post error");                          /* Unlock the buffer */
    if (sem_post(&sp->items) < 0) printf("sem_post error");                          /* Announce available item */
}

char* logbuf_remove(logbuf_t *sp)
{
    char* item;
    if (sem_wait(&sp->items) < 0) printf("sem_wait error");                          /* Wait for available item */
    if (sem_wait(&sp->mutex) < 0) printf("sem_wait error");                         /* Lock the buffer */
    item = sp->buf[(++sp->front)%(sp->n)];  /* Remove the item */
    if (sem_post(&sp->mutex) < 0) printf("sem_post error");                         /* Unlock the buffer */
    if (sem_post(&sp->slots) < 0) printf("sem_post error");                          /* Announce available slot */
    return item;
}