#include "my_malloc.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sbrklock = PTHREAD_MUTEX_INITIALIZER;


typedef struct List_Node ListNode;
//Using Linked list to manage the memory
struct List_Node{
  ListNode* next;//8 bytes
  ListNode* prev;//8 bytes
  size_t size;//8 bytes
};
////////////////////////////////////////////////////////////
//Locking version

//The head of the free List
ListNode* head = NULL;
__thread ListNode* head_nolock = NULL;

//If there is still enough space established, split it into two parts
ListNode* split(ListNode* current, size_t size){
  current->size -= sizeof(ListNode);
  current->size -= size;
  ListNode * temp = (ListNode*)((char*)current+ sizeof(ListNode) + current->size);
  temp->size = size;
  return temp;
}

//Establish New nodes if there is not enough space
void* establish(size_t size){
  ListNode* current = sbrk(sizeof(ListNode) + size);
   //Update global variable
  current->size = size;
  return current;
 }

//Find the first fit block currently in the linked list, allocate an
//address from the first free region with enough space to fit the requested allocation size.
ListNode *  ff_find(size_t size){
  ListNode * current = head;
    while (current){
      if( current->size >= size){
        return current;
      }
        current = current->next;
      }
    return current;
}

//This function will help to merge the neighboring free blocks which
//can solve the problem of poor region selection during malloc.
ListNode* merge(ListNode * current){
     current->size += sizeof(ListNode);
    current->size += current->next->size;
    current->next = current->next->next;
      if (current->next){
            current->next->prev= current;
        }
   return current;
}

//Adding a new free bolck to the free list
void add_free(void*ptr){
  ListNode * temp = (ListNode*)(ptr - sizeof(ListNode));
  if(head == NULL){
    head = temp;
    head->next = NULL;
    head->prev = NULL;
  }
  else{
    if(temp < head){
        temp->next = head;
        temp->prev = NULL;
        head = temp;
        return;
      }
    ListNode * last = head;
    ListNode * current = head;
    while(current != NULL){
      if(current < temp ){
        if(current->next && temp < current->next){
          temp->next = current->next;
          current->next = temp;
          temp->next->prev = temp;
          temp->prev = current;
          return;
        }
      }
        last = current;
        current= current->next;
    }
      last->next = temp;
      temp->prev = last;
      temp->next = NULL;
  }
}

//The function bf_find( ) will find the best fit free node in the
//linked list when a new chunk of memory need to be malloced, allocate
//an address from the free region which has the smallest number of
// bytes greater than or equal to the requested allocation size.
ListNode* bf_find(size_t size){
  ListNode* current = head;
  ListNode* best = NULL;
  int rest = 0;
  int min_rest = 0;
  //Get first data for min_rest
  while (current){
      rest = current->size - size;
      if(rest == 0){
        return current;
      }
      if(rest > 0 ){
        min_rest = rest;
        best = current;
        break;
      }
    current = current->next;
    }
  //If there cannot find such node
  if(current == NULL){
    return current;
  }
  current = head;
  //Find the best fit node
  while (current){
    rest = current->size - size;
        if(rest == 0){
          return current;
        }
        if(rest > 0 && rest < min_rest){
          best = current;
          min_rest = rest;
        }
      current = current->next;
  }
  return best;
}

//Best Fit malloc/free
void *ts_malloc_lock(size_t size){
pthread_mutex_lock(&lock);
  ListNode * current;
  ListNode * malloced;
       if ( head ){
           if ( (current = bf_find(size)) ){
        //Is able to split
                if (current->size - size > sizeof(ListNode)){
               malloced =  split(current, size);
           }
        //Found the node and use it without split
                else{
                  if(current->prev == NULL && current->next == NULL){
                    head = NULL;
                    malloced = current;
                  }
                 else if(current->prev == NULL){
                   head = current->next;
                   current->next->prev = NULL;
                   malloced = current;
                  }
                  else if(current->next == NULL){
                    current->prev->next = NULL;
                    malloced = current;
                  }
                  else{
                    current->prev->next = current->next;
                    current->next->prev = current->prev;
                    malloced = current;
                  }
                }
           }
             else {
        //Establish a note at the end
               malloced = establish(size);
      }
       }
       else{
         //First time establish
         malloced = establish(size);
         }
       pthread_mutex_unlock(&lock);
       return (char*)malloced + sizeof(ListNode);
}

void ts_free_lock(void *ptr){
pthread_mutex_lock(&lock);
  ListNode*  current = (ListNode*)((char*)(ptr) - sizeof(ListNode));
    add_free(ptr);
    if (current->next && ((char*)current + sizeof(ListNode) + current->size  == (char*)current->next)){
       merge(current);
    }
    if (current->prev && ((char*)current - current->prev->size - sizeof(ListNode) ==(char*) current->prev)){
      merge(current->prev);
    }
    pthread_mutex_unlock(&lock);
}

///////////////////////////////////////////////////////////////////////////////
//No lock version
//Establish New nodes if there is not enough space
void* establish_nolock(size_t size){
pthread_mutex_lock(&sbrklock);
  ListNode* current = sbrk(sizeof(ListNode) + size);
  pthread_mutex_unlock(&sbrklock);
   //Update global variable
  current->size = size;
  return current;
 }

//Adding a new free node to the free list
void add_free_nolock(void*ptr){
  ListNode * temp = (ListNode*)(ptr - sizeof(ListNode));
  if(head_nolock == NULL){
    head_nolock = temp;
    head_nolock->next = NULL;
    head_nolock->prev = NULL;
  }
  else{
    if(temp < head_nolock){
        temp->next = head_nolock;
        temp->prev = NULL;
        head_nolock = temp;
        return;
      }
    ListNode * last = head_nolock;
    ListNode * current = head_nolock;
    while(current != NULL){
      if(current < temp ){
        if(current->next && temp < current->next){
          temp->next = current->next;
          current->next = temp;
          temp->next->prev = temp;
          temp->prev = current;
          return;
        }
      }
        last = current;
        current= current->next;
    }
      last->next = temp;
      temp->prev = last;
      temp->next = NULL;
  }
}

//The function bf_find( ) will find the best fit free node in the
//linked list when a new chunk of memory need to be malloced, allocate
//an address from the free region which has the smallest number of
// bytes greater than or equal to the requested allocation size.
ListNode* bf_find_nolock(size_t size){
  ListNode* current = head_nolock;
  ListNode* best = NULL;
  int rest = 0;
  int min_rest = 0;
  //Get first data for min_rest
  while (current){
      rest = current->size - size;
      if(rest == 0){
        return current;
      }
      if(rest > 0 ){
        min_rest = rest;
        best = current;
        break;
      }
    current = current->next;
    }
  //If there cannot find such node
  if(current == NULL){
    return current;
  }
  current = head_nolock;
  //Find the best fit node
  while (current){
    rest = current->size - size;
        if(rest == 0){
          return current;
        }
        if(rest > 0 && rest < min_rest){
          best = current;
          min_rest = rest;
        }
      current = current->next;
  }
  return best;
}


void *ts_malloc_nolock(size_t size){
 ListNode * current;
  ListNode * malloced;
       if ( head_nolock ){
           if ( (current = bf_find_nolock(size)) ){
        //Is able to split
                if (current->size - size > sizeof(ListNode)){
               malloced =  split(current, size);
           }
        //Found the node and use it without split
                else{
                  if(current->prev == NULL && current->next == NULL){
                    head_nolock = NULL;
                    malloced = current;
                  }
                 else if(current->prev == NULL){
                   head_nolock = current->next;
                   current->next->prev = NULL;
                   malloced = current;
                  }
                  else if(current->next == NULL){
                    current->prev->next = NULL;
                    malloced = current;
                  }
                  else{
                    current->prev->next = current->next;
                    current->next->prev = current->prev;
                    malloced = current;
                  }
                }
           }
             else {
        //Establish a note at the end
               malloced = establish_nolock(size);
      }
       }
       else{
         //First time establish
         malloced = establish_nolock(size);
         }
       return (char*)malloced + sizeof(ListNode);
}

void ts_free_nolock(void *ptr){
ListNode*  current = (ListNode*)((char*)(ptr) - sizeof(ListNode));
    add_free_nolock(ptr);
    if (current->next && ((char*)current + sizeof(ListNode) + current->size  == (char*)current->next)){
       merge(current);
    }
    if (current->prev && ((char*)current - current->prev->size - sizeof(ListNode) ==(char*) current->prev)){
      merge(current->prev);
    }
}

