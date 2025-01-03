#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "vm/page.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

struct thread *get_child_thread(tid_t child_tid);


bool page_handle(struct page *p);
bool expand_stack(void *addr, void *esp);
#endif /* userprog/process.h */