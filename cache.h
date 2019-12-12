#ifndef MARJ_CACHE_H
#define MARJ_CACHE_H

#include <stdlib.h>
#include "csapp.h"

typedef struct CachedItem CachedItem;

struct CachedItem {
  char url[MAXLINE];
  void *item_p;
  size_t size;
  CachedItem *prev;
  CachedItem *next;
};

typedef struct {
  size_t size;
  CachedItem* first;
  CachedItem* last;
} CacheList;

extern void cache_init(CacheList *list);

extern void cache_URL(char *URL, void *item, size_t size, CacheList *list);

extern void evict(CacheList *list);

extern CachedItem *find(char *URL, CacheList *list);

extern void move_to_front(char *URL, CacheList *list);

extern void print_URLs(CacheList *list);

extern void cache_destruct(CacheList *list);
//Free the cache, start at end then go prev and free as you go
#endif