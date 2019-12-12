  
#ifndef __LOGBUF_H__
#define __LOGBUF_H__

#include "csapp.h"

typedef struct {
    char **buf;          /* Buffer array */         
    int n;             /* Maximum number of slots */
    int front;         /* buf[(front+1)%n] is first item */
    int rear;          /* buf[rear%n] is last item */
    sem_t mutex;       /* Protects accesses to buf */
    sem_t slots;       /* Counts available slots */
    sem_t items;       /* Counts available items */
} logbuf_t;

void logbuf_init(logbuf_t *sp, int n);
void logbuf_deinit(logbuf_t *sp, int n);
void logbuf_insert(logbuf_t *sp, char* item);
char* logbuf_remove(logbuf_t *sp);

#endif /* __SBUF_H__ */