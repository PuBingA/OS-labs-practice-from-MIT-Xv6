#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);

  //自己添加对 backtrace的调用
  backtrace();
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

//按要求只返回0
uint64 sys_sigreturn(void)
{
  struct proc *p=myproc();
  memmove(p->trapframe, p->copy_trapframe, sizeof(struct trapframe));//拷贝trapframe
  p->passtime=0;
  p->copy_trapframe=0; //重置相关参数

  return p->trapframe->a0;
}

//添加 sigalarm函数，读取时间间隔和handler指针
uint64 sys_sigalarm(void)
{
  int interval;
  uint64 handler;
  struct proc *p;

  argint(0,&interval);
  argaddr(1,&handler);//读取时间间隔和指针

  p=myproc();
  p->interval=interval;
  p->handler=handler;

  return 0;

}
