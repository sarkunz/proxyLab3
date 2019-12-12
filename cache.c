#include "cache.h"
#include <assert.h>

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400


void cache_init(CacheList *list){
  list->size = 0;
  list->first = NULL;
  list->last = NULL;
}

void cache_URL(char *URL, void *item, size_t size, CacheList *list){
  //check object size
  if (size > MAX_OBJECT_SIZE){
    return;
  }

  //check space in linked list, evict while necessary
  while((list->size + size) > MAX_CACHE_SIZE){
    evict(list);
  }

  list->size += size;

  CachedItem* cached_item = (CachedItem*) malloc(sizeof(struct CachedItem));

  strcpy(cached_item->url, URL);
  cached_item->item_p = item;
  cached_item->size = size;

  //If list is empty, set first and last item in list to item
  if(list->first == NULL){
    list->first = cached_item;
    list->last = cached_item;
  }
  else{
    //Non empty list
    //reorder so item is first and original first is now next
    list->first->prev = cached_item;
    cached_item->next = list->first;
    cached_item->prev = NULL;
    list->first = cached_item;
  }

  return;
}

void evict(CacheList *list){

  assert(list->size > 0);
  assert(list->first != NULL);
  assert(list->last != NULL);

  //if it's last last item the list
  if(list->last->size == list->size){
    free(list->last->item_p);
    free(list->last);
    list->last = NULL;
    list->first = NULL;
    list->size = 0;
    return;
  }

  assert(list->last != list->first);

  //Remove any item from list except last item
  list->last = list->last->prev;
  list->size -= list->last->next->size;
  free(list->last->next->item_p);
  free(list->last->next);
  list->last->next = NULL;
  return;
}

CachedItem* find(char *URL, CacheList *list){
  //list is empty, return NULL

  if(list->size > 0){
    if(strcmp(list->first->url, URL) == 0){
      return list->first;
    }
    //Check if the last item in list is it
    if(strcmp(list->last->url, URL) == 0){
      return list->last;
    }

    CachedItem* temp = list->first;
    while(temp->next != NULL){
      if(strcmp(temp->url, URL) != 0){
        temp = temp->next;
      }
      else{
        return temp;
      }
    }
  }
  return NULL;
}

void move_to_front(char *URL, CacheList *list){
  CachedItem* item = find(URL, list);
  //Item dne
  if(item == NULL)
    return;

  //Check if it's first in list
  if(item == list->first)
    return;

  //Check if its last in list
  if(item == list->last){
    list->last = item->prev;
    list->first->prev = item;
    item->next = list->first;
    item->prev = NULL;
    list->last->next = NULL;
    list->first = item;
    return;
  }
  else{
    //its in the middle
    item->prev->next = item->next;
    item->next->prev = item->prev;
    item->prev = NULL;
    item->next = list->first;
    list->first->prev = item;
    list->first = item;
    return;
  }

}

void print_URLs(CacheList *list){
  if(list->size > 0){
    CachedItem* item = list->first;
    while(item->next != NULL){
      printf("%s/n", item->url);
      item = item->next;
    }
    printf("%s\n", item->url);
  }
}

void cache_destruct(CacheList *list){
  while(list->size != 0)
    evict(list);
}
//Free the cache, start at end then go prev and free as you go