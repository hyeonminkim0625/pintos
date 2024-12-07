#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "vm/page.h"
#include "vm/frame.h"
#include <string.h>
#include "threads/malloc.h"


static void syscall_handler (struct intr_frame *f UNUSED);
struct lock filelock;
struct file *get_file(int fd);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&filelock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  // printf("syscall!\n");
  int argv[3];
  void *esp = f->esp;
  int sys_num = *(int *) (f->esp);
  thread_current()->esp = esp;
  check_addr(esp);

  for(int i = 0; i < 3; i++){
    check_addr(esp + 4*(i+1));
  }

  for(int i = 0; i < 3; i++){
    argv[i] = *(int *)(esp + 4*(i+1)); 
  }

  switch(sys_num)
  {
    case SYS_HALT:
    {
      halt();
      break;
    }
    case SYS_EXIT:
    {
      exit((int) argv[0]);
      break;
    }
    case SYS_EXEC:
    {
      f->eax = exec((const char*)argv[0]);
      break;
    }
    case SYS_WAIT:
    {
      f->eax = process_wait((tid_t) argv[0]);
      break;
    }
    case SYS_CREATE:
    {
      f->eax = create((const char*)argv[0], (unsigned int) argv[1]);
      break;
    }
    case SYS_REMOVE:
    {
      f->eax = remove((const char*) argv[0]);
      break;
    }
    case SYS_OPEN:
    {
      f->eax = open((const char *) argv[0]);
      break;
    }
    case SYS_FILESIZE:
    {
      f->eax = filesize((int) argv[0]);
      break;
    }
    case SYS_READ:
    {
      f->eax = read((int) argv[0], (void *) argv[1], (unsigned) argv[2]);
      break;
    }
    case SYS_WRITE:
    {
      f->eax = write((int) argv[0], (const void *) argv[1], (unsigned) argv[2]);
      break;
    }
    case SYS_SEEK:
    {
      seek((int) argv[0], (unsigned) argv[1]);
      break;
    }
    case SYS_TELL:
    {
      f->eax = tell((int) argv[0]);
      break;
    }
    case SYS_CLOSE:
    {
      close((int) argv[0]);
      break;
    }
    case SYS_MMAP:
    {
      f->eax = mmap((int) argv[0], (void *) argv[1]);
      break;
    }
    case SYS_MUNMAP:
    {
      munmap((int) argv[0]);
      break;
    }
    default:
    {
      exit(-1);
    }
  }
}

void 
check_addr(void *addr)
{
  if(!is_user_vaddr(addr) || addr == NULL)
  {
    exit(-1);
  }
}

void
halt(void)
{
  shutdown_power_off();
}

void 
exit(int status)
{
  thread_current()->exit_code = status;
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_exit();
}

pid_t
exec (const char *cmd_line)
{
  // printf("exec~~!!\n");
  tid_t tid;
  int len = strlen(cmd_line) + 1;
  char *fn_copy = palloc_get_page(0);
  if(!fn_copy) exit(-1);
  strlcpy(fn_copy, cmd_line, len);
  tid = process_execute(fn_copy);
  if (tid == -1) {
    palloc_free_page(fn_copy);  // fn_copy 메모리 해제
    return -1;
  }

  struct thread *child = get_child_thread(tid);
  if (child == NULL) {
    palloc_free_page(fn_copy);
    return -1;
  }

  sema_down(&child->exec);  // 자식이 로드될 때까지 대기
  if (!child->loading) {     // 자식이 로드 실패한 경우
    palloc_free_page(fn_copy);
    return -1;
  }
  palloc_free_page(fn_copy);
  return tid;
}

bool
create (const char *file, unsigned initial_size)
{
  bool success;
  check_addr((void*)file);
  lock_acquire(&filelock);
  success = filesys_create(file, initial_size);
  lock_release(&filelock);
  return success;
}

bool
remove (const char *file)
{
  bool success = false;
  check_addr((void*)file);
  lock_acquire(&filelock);
  success = filesys_remove(file);
  lock_release(&filelock);
  return success;
}

int
open (const char *file)
{
  check_addr((void*)file);
  lock_acquire(&filelock);
  struct file *f = filesys_open(file);
  if(f==NULL)
  {//not open
    lock_release(&filelock);
    return -1;
  }
  if(!strcmp(thread_current()->name, file))
  {
    file_deny_write(f);
  }
  struct thread *cur = thread_current();
  int fd = cur->filecount;
  cur->file_list[fd] = f; // 몇번째 파일 디스크립터인지 fd에 저장
  cur->filecount++;
  lock_release(&filelock);
  return fd;
}

int
filesize (int fd)
{
  lock_acquire(&filelock);
  struct file *f = get_file(fd);
  if(f==NULL)
  {
    lock_release(&filelock);
    return -1;
  }
  else
  {
    int size = file_length(f);
    lock_release(&filelock);
    return size;
  }
}

int
read (int fd, void *buffer, unsigned size)
{
  int bytes = 0;
  unsigned i;
  for(i = 0; i < size; i++)
    check_addr(buffer + i);

  if (fd<0)
  {
    return -1;
  }
  else if(fd==0)
  {
    for(i=0; i < size; i++)
    {
      lock_acquire(&filelock);
      *(char *)(buffer+i) = input_getc();
      lock_release(&filelock);
      bytes++;
    }
  }
  else
  {
    struct file *f = get_file(fd);
    if(f==NULL) 
      return -1;

    lock_acquire(&filelock);
    bytes = file_read(f, buffer, size);
    lock_release(&filelock);

  }
  return bytes;
}

int
write (int fd, const void *buffer, unsigned size)
{
  int byte = 0;
  unsigned i;
  for(i = 0; i < size; i++)
    check_addr(buffer+i);

  if(fd == 1)
  {
    lock_acquire(&filelock);
    putbuf(buffer, size);
    lock_release(&filelock);
    byte = size;
  }
  else
  {
    struct file *f = get_file(fd);
    if(f == NULL) return -1;
    lock_acquire(&filelock);
    byte = file_write(f,buffer,size);
    lock_release(&filelock);
  }
  return byte;
}

void
seek (int fd, unsigned position)
{
  lock_acquire(&filelock);
  struct file *f = get_file(fd);
  ASSERT(f != NULL);
  file_seek(f, position);
  lock_release(&filelock);
}

unsigned
tell(int fd)
{
  lock_acquire(&filelock);
  struct file *f = get_file(fd);
  if(f==NULL)
  {
    lock_release(&filelock);
    return -1;
  }
  else
  {
    unsigned pos = file_tell(f);
    lock_release(&filelock);
    return pos;
  }
}

void
close (int fd)
{
  if(!lock_held_by_current_thread(&filelock))
    lock_acquire(&filelock);

  struct file *f = get_file(fd);
  if(f)
  {
    file_close(f);
    thread_current()->file_list[fd] = NULL;
  }
  lock_release(&filelock);
}

struct file 
*get_file(int fd)
{
  if(fd < thread_current()->filecount &&  fd > 1)
    return thread_current()->file_list[fd];
  return NULL;
}

int
mmap(int fd, void *addr)
{
  struct file *f;
  int size;
  struct mmap_file *new_mmap;
  int offset = 0;
  struct thread *cur = thread_current();

  if(!is_user_vaddr(addr))
    exit(-1);

  if(pg_ofs(addr) != 0 || !addr)
    return -1;

  if(fd < 2)
    return -1;
  
  f = get_file(fd);
  if(!f)
    return -1;

  lock_acquire(&filelock);
  f = file_reopen(f);
  lock_release(&filelock);
  if(!f)
    return -1;
  
  lock_acquire(&filelock);
  size = file_length(f);
  lock_release(&filelock);
  if(!size)
    return -1;
  
  new_mmap = malloc(sizeof(struct mmap_file));
  if (!new_mmap)
    return -1;
  memset(new_mmap, 0 , sizeof(struct mmap_file));
  
  list_init(&new_mmap->page_list);
  while(size > 0)
  {
    if (page_find(addr)) 
      return -1;

    size_t page_read_bytes = size < PGSIZE ? size : PGSIZE;
    size_t page_zero_bytes = PGSIZE - page_read_bytes;

    struct page* new_page = make_page(PG_W, addr, true, false, f, offset, page_read_bytes, page_zero_bytes);
    if (!new_page) 
      return false;

    list_push_back(&new_mmap->page_list, &new_page->mmap_elem);
    spt_insert(&cur->spt, new_page);
		
    addr += PGSIZE;
    offset += PGSIZE;
    size -= PGSIZE;
  }
  cur->mmapcount += 1;
  list_push_back(&cur->mmap_list, &new_mmap->elem);
  new_mmap->mapid = cur->mmapcount;
  new_mmap->file = f;
  return new_mmap->mapid;
}

void
munmap(int mapid)
{
  struct thread *t = thread_current();
  struct list_elem *e;
  struct page *p;
  struct mmap_file *temp = NULL;

  for(e = list_begin(&t->mmap_list); e != list_end(&t->mmap_list); e = list_next(e))
  {
    temp = list_entry(e, struct mmap_file, elem);
    if(temp->mapid == mapid)
      break;
  }
  if(!temp)
    return;

  for(e = list_begin(&temp->page_list); e != list_end(&temp->page_list);)
  {
    p = list_entry(e, struct page, mmap_elem);
    if(p->load)
    {
      if(pagedir_is_dirty(t->pagedir, p->va))
      {
        lock_acquire(&filelock);
        file_write_at(p->f, p->va, p->read_bytes, p->offset);
        lock_release(&filelock);
        // lock_acquire(&ft_lock);
        // void *addr = pagedir_get_page(t->pagedir, p->va);
        // lock_release(&ft_lock);
        // free_frame(addr);
      }
    }
    p->load = false;
    e = list_remove(e);
    spt_delete(&t->spt, p);
  }
  list_remove(&temp->elem);
  free(temp);
}