#include "vm/frame.h"
#include "threads/synch.h"
#include "userprog/pagedir.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "filesys/file.h"
#include "vm/page.h"
#include "vm/swap.h"

#include <list.h>
#include <string.h>

extern struct lock filelock;

void
frame_table_init(void)
{
    list_init(&ft);
    lock_init(&ft_lock);
    clock_point = NULL;
}

struct frame*
allocate_frame(enum palloc_flags flags)
{
    ASSERT(flags & PAL_USER);
    struct frame *new_frame = malloc(sizeof(struct frame));
    if (!new_frame)
        return NULL;
    
    memset(new_frame, 0, sizeof(struct frame));
    new_frame->t = thread_current();

    lock_acquire(&ft_lock);
    new_frame->va = palloc_get_page(flags);

    while(!new_frame->va)
    {
        evict_frame();
        new_frame->va = palloc_get_page(flags);
    }

    if(list_empty(&ft)) 
        clock_point = &new_frame->frame_elem;

    list_push_back(&ft, &new_frame->frame_elem);
    lock_release(&ft_lock);
    return new_frame;
}

void 
free_frame(void *addr)
{
    struct list_elem *e;
    struct frame *temp;
    struct frame *to_free = NULL;

    for(e = list_begin(&ft); e != list_end(&ft); e = list_next(e))
    {
        temp = list_entry(e, struct frame, frame_elem);
        if(temp->va == addr)
        {
            temp->page_ptr->load = false;
            pagedir_clear_page(temp->t->pagedir, temp->page_ptr->va);
            palloc_free_page(temp->va);

            lock_acquire(&ft_lock);
            if(e == clock_point) 
                clock_point = list_next(clock_point);
            list_remove(e);
            to_free = temp;
            lock_release(&ft_lock);
            break;
        }
    }

    if (to_free) 
    {
        free(to_free);
    }
}

void
evict_frame(void)
{
    bool dirty;
    struct frame *f = select_victim_frame();
    if (f == NULL) return;

    dirty = pagedir_is_dirty(f->t->pagedir, f->page_ptr->va);
    switch (f->page_ptr->page_type)
    {
        case PG_D:
            if(dirty)
            {
                f->page_ptr->slot = swap_out(f->page_ptr);
                f->page_ptr->page_type = PG_S;
            }
            break;
        case PG_W:
            if(dirty)
            {
                lock_acquire(&filelock);
                file_write_at(f->page_ptr->f, f->va, f->page_ptr->read_bytes, f->page_ptr->offset);
                lock_release(&filelock);
            }
            break;
        case PG_S:
            f->page_ptr->slot = swap_out(f->page_ptr);
            break;
    }
    
    pagedir_clear_page(f->t->pagedir, f->page_ptr->va);
    palloc_free_page(f->va);
    clock_point = list_remove(&(f->frame_elem));
    f->page_ptr->load = false;
    free(f);
}

struct frame*
select_victim_frame(void)
{
    struct frame *temp;
    if(list_empty(&ft)) return NULL;

    while(true)
    {

        if(clock_point == list_end(&ft))
            clock_point = list_begin(&ft);

        else
        {
            clock_point = list_next(clock_point);
            if(clock_point == list_end(&ft)) 
                continue;
        }
        
        temp = list_entry(clock_point, struct frame, frame_elem);
        
        if(pagedir_is_accessed(temp->t->pagedir, temp->page_ptr->va))
            pagedir_set_accessed(temp->t->pagedir, temp->page_ptr->va, false);

        else
            return temp;
    }
}