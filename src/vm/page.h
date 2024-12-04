#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "filesys/file.h"
#include <hash.h>
#include <list.h>

enum page_type {
    PG_D,
    PG_W,
    PG_S
};

struct page
{
    enum page_type page_type;
    void* va;
    bool load;
    bool write;
    struct file *f;

    size_t offset;
    size_t read_bytes;
    size_t zero_bytes;
    struct hash_elem elem;
};


void print_hash(struct hash *h);
void spt_init(struct hash *h);
struct page *make_page(uint8_t type, void *va, bool write, bool load, struct file* file, size_t offset, size_t read_bytes, size_t zero_bytes);
static unsigned spt_hash_func(const struct hash_elem *e, void *aux);
static bool spt_less_func(const struct hash_elem *a,const struct hash_elem *b, void *aux);
bool spt_insert(struct hash *h, struct page *p);
bool spt_delete(struct hash *h, struct page *p);
struct page *page_find(void *addr);
void spt_delete_func(struct hash_elem *e, void *aux);
void spt_destroy (struct hash *h);
bool load_file(void *addr, struct page *p);
#endif /* vm/page.h */