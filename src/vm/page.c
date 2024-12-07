#include "vm/page.h"
#include "vm/frame.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"
#include "filesys/file.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include <hash.h>
#include <string.h>
#include <stdio.h>

extern struct lock filelock;

void
print_hash(struct hash *h)
{
    struct hash_iterator i;
    struct thread *cur = thread_current();
    hash_first(&i, h);
    printf("thread name : %s\n", cur->name);
    while (hash_next(&i))
    {
        struct page *p = hash_entry(hash_cur(&i), struct page, elem);
        printf("Page va: %p\n", p->va);
        printf("Pagedir get page : %p\n", pagedir_get_page(cur->pagedir, p->va));
    }
}


struct page
*make_page(uint8_t type, void *va, bool write, bool load, struct file* file, size_t offset, size_t read_bytes, size_t zero_bytes)
{
    struct page *p = malloc(sizeof(struct page));
    if(!p) return NULL;
    memset(p, 0, sizeof(struct page));

    p->page_type = type;
    p->va = va;
    p->load = load;
    p->write = write;
    p->f = file;
    p->offset = offset;
    p->read_bytes = read_bytes;
    p->zero_bytes = zero_bytes;

    return p;
}


static unsigned
spt_hash_func(const struct hash_elem *e, void *aux)
{
    struct page *p = hash_entry(e, struct page, elem);
    return hash_int((int)p->va);
}

static bool
spt_less_func(const struct hash_elem *a,const struct hash_elem *b, void *aux)
{
  return hash_entry(a, struct page, elem)->va < hash_entry(b, struct page, elem)->va;
}

void 
spt_init(struct hash *h)
{
    hash_init(h, spt_hash_func, spt_less_func, NULL);
}

bool
spt_insert(struct hash *h, struct page *p)
{
    bool success;
    lock_acquire(&ft_lock);
    success = hash_insert(h,&p->elem) == NULL ? true : false;
    lock_release(&ft_lock);
    return success;
}

bool
spt_delete(struct hash *h, struct page *p)
{
    struct thread *cur = thread_current();

    lock_acquire(&ft_lock);
    void *addr = pagedir_get_page(cur->pagedir, p->va);
    lock_release(&ft_lock);

    if (hash_delete(h,&p->elem))
    {
        free_frame(addr);
        free(p);
        return true;
    }
    return false;
}

struct page
*page_find(void *addr)
{
    struct hash *h = &thread_current()->spt;
    // struct page p;
    struct hash_elem *e = NULL;

    // p.va = pg_round_down(addr);
    // e = hash_find(h, &p.elem);
    struct hash_iterator i;
    struct thread *cur = thread_current();
    hash_first(&i, h);

    while (hash_next(&i))
    {
        struct page *p = hash_entry(hash_cur(&i), struct page, elem);
        if(p->va == pg_round_down(addr))
        {
            e = &p->elem;
            break;
        }
    }

    return e != NULL ? hash_entry(e, struct page, elem) : NULL;
}

void
spt_delete_func(struct hash_elem *e, void *aux)
{
    struct page *p = hash_entry(e, struct page, elem);
    struct thread *cur = thread_current();

    lock_acquire(&ft_lock);
    void *addr = pagedir_get_page(cur->pagedir, p->va);
    lock_release(&ft_lock);

    if(p != NULL)
    {
        if(p->load)
        {
            free_frame(addr);
        }
        free(p);
    }
}

void 
spt_destroy(struct hash *h) 
{
    hash_destroy(h, spt_delete_func);
    // struct hash_iterator i;
    // struct thread *cur = thread_current();

    // // 초기 상태 출력
    // print_hash(h);

    // // 모든 요소를 안전하게 순회하며 제거
    // hash_first(&i, h);
    // while (hash_next(&i)) 
    // {
    //     struct page *p = hash_entry(hash_cur(&i), struct page, elem);
    //     if (p != NULL) 
    //     {
    //         if (p->load) 
    //         {
    //             hash_delete(h, &p->elem);
    //             free_frame(pagedir_get_page(cur->pagedir, p->va));
    //         }
    //         free(p);
    //     }
    // }
}



bool 
load_file(void *addr, struct page *p)
{
    size_t bytes;
    if(lock_held_by_current_thread(&filelock))
    {
        bytes = file_read_at(p->f, addr, p->read_bytes, p->offset);
    }
    else
    {
        lock_acquire(&filelock);
        bytes = file_read_at(p->f, addr, p->read_bytes, p->offset);
        lock_release(&filelock);
    }
    if(bytes != (int)p->read_bytes)
        return false;
    
    memset(addr + bytes, 0, p->zero_bytes);
    return true;
}