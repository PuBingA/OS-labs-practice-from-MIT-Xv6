// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];  //每个CPU分配的链表核锁,改成数组形式,数组容量为CPU个数

void
kinit()
{
  for(int i=0;i<NCPU;i++)
  {
    char str[10];
	  snprintf(str,9,"kmem %d",i); //hint最后一条要求的格式化输出
	  initlock(&kmem[i].lock, str);//每个CPU都分配锁
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  //hint指出要先关闭中断，才能获取id
  push_off();
  int id= cpuid();
  pop_off();

  acquire(&kmem[id].lock);
  r->next = kmem[id].freelist;
  kmem[id].freelist = r;
  release(&kmem[id].lock);
}



 //借用前半部分页
struct run *borrow(int id){
	struct run *r,*slow, *fast;
	for(int i=0;i<NCPU;i++){
		if(i == id)
			continue;
		acquire(&kmem[i].lock);
		if(kmem[i].freelist){
     		slow = kmem[i].freelist;fast = kmem[i].freelist->next; //快慢指针 将链表化为两部分
     		r = slow;
     		if(fast == 0||fast->next == 0){
     			release(&kmem[i].lock);
     			continue;
     		}
     		while(fast != slow){
     			if(fast == 0||fast->next == 0)
     				break;
     			slow = slow->next;
     			fast = fast->next->next;
     		}
			r = slow->next;
     		slow->next = 0;
     		release(&kmem[i].lock);
     		return r;

		}
		release(&kmem[i].lock);
	}
	return 0;
}


// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  
  //获得id安全操作
  push_off();
  int id = cpuid();
  pop_off();

  acquire(&kmem[id].lock);
  r=kmem[id].freelist;
  if(r)
    kmem[id].freelist = r->next;
  else //如果没有空闲块，就要借用其他 kmem中链表空闲块
   r=borrow(id);
  

  if(r)
    kmem[id].freelist = r->next;
  release(&kmem[id].lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
