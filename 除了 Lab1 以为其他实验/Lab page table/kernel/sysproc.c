#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
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
  return 0;
}


//修改代码实现 detect



extern pte_t * walk(pagetable_t, uint64, int);
int
sys_pgaccess(void)
{
  uint64 bitmask=0;
  int length;
  uint64 start;
  uint64 bit;

  if(argint(1,&length)<0)
  return -1; //获取页面数量

  if(length>32)
  return -1; //超过最大数32退出

  if(argaddr(0,&start)<0)
  return -1;
  if(argaddr(2,&bit)<0)
  return -1;
  //获取地址和指针

  int i;
  pte_t* pte;
  
   for(i = 0 ; i <length ; start += PGSIZE, ++i)
   {
    if((pte = walk(myproc()->pagetable, start, 0)) == 0)
      panic("pgaccess : walk failed");
    if(*pte & PTE_A)
    {
      bitmask |= 1 << i;	// 设置BitMask对应位
      *pte &= ~PTE_A;		// 将PTE_A清空
    }
  }


    copyout(myproc()->pagetable, bit, (char*)&bitmask, sizeof(bitmask)); //传给用户
  return 0;

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
