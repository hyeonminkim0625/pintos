#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
typedef int pid_t;

void syscall_init (void);
void check_addr(void *addr);


void halt(void);
void exit(int status);
pid_t exec (const char *cmd_line);
#endif /* userprog/syscall.h */