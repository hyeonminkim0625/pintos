#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <stdbool.h>
typedef int pid_t;



void syscall_init (void);
void check_addr(void *addr);
void halt(void);
void exit(int status);
pid_t exec (const char *cmd_line);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell(int fd);
void close (int fd);
int mmap(int fd, void *addr);
void munmap(int mapid);

#endif /* userprog/syscall.h */