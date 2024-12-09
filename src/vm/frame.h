#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/palloc.h"
#include "vm/page.h"
#include <list.h>

struct list ft;
static struct list_elem *clock_point;
struct lock ft_lock;

struct frame
  {
    struct thread *t;                
    struct list_elem frame_elem;     
    void *va;
    struct page *page_ptr;
  };

void frame_table_init(void);
struct frame* allocate_frame(enum palloc_flags flags);
void free_frame(void *addr);
void evict_frame(void); 
struct frame* select_victim_frame(void);


#endif /* vm/frame.h */