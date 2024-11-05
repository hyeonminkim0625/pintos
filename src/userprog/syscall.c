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
#include "threads/palloc.h"
#include <string.h>


static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int argv[3];
  void *esp = f->esp;
  int sys_num = *(int *) (f->esp);
  check_addr(esp);

  for(int i = 0; i < 3; i++){
    argv[i] = *(int *)(esp + 4*i); 
  }

  for(int i = 0; i < 3; i++){
    check_addr(esp + 4*i);
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
      int status = *(int *)(esp + 4);
      exit((int) status);
      break;
    }
    case SYS_EXEC:
    {
      pid_t pid = exec((const char*)argv[0]);
      if(pid == -1) exit(-1);
      break;
    }
    case SYS_WAIT:
    {
      exit(-1);
      break;
    }
    case SYS_CREATE:
    {
      exit(-1);
      break;
    }
    case SYS_REMOVE:
    {
      exit(-1);
      break;
    }
    case SYS_OPEN:
    {
      exit(-1);
      break;
    }
    case SYS_FILESIZE:
    {
      exit(-1);
      break;
    }
    case SYS_READ:
    {
      exit(-1);
      break;
    }
    case SYS_WRITE:
    {
      exit(-1);
      break;
    }
    case SYS_SEEK:
    {
      exit(-1);
      break;
    }
    case SYS_TELL:
    {
      exit(-1);
      break;
    }
    case SYS_CLOSE:
    {
      exit(-1);
      break;
    } 
    case SYS_MMAP:
    {
      exit(-1);
      break;
    }
    case SYS_MUNMAP:
    {
      exit(-1);
      break;
    }
    default:
      exit(-1);
  }
}

void 
check_addr(void *addr)
{
  if(!is_user_vaddr(addr) || !addr)
    exit(-1);
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
  int len = strlen(cmd_line) + 1;
  char *fn_copy = palloc_get_page(0);
  if(!fn_copy) exit(-1);
  strlcpy(fn_copy, cmd_line, len);

  return process_execute(fn_copy);
}