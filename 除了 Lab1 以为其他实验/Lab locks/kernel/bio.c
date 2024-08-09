// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"



#define buckets_sum 13 //桶数量
#define HASH(blockno) (blockno % buckets_sum )



struct {
  struct spinlock lock;   // used for the buf alloc and size
  struct buf buf[NBUF];
  int size;     // record the count of used buf - lab8-2
  struct buf buckets[buckets_sum];
  struct spinlock locks[buckets_sum];   
  struct spinlock hashlock;     
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
//  struct buf head;    

} bcache;

 
void
binit(void)
{
  struct buf *b;

  bcache.size = 0;  
  initlock(&bcache.lock, "bcache");
  initlock(&bcache.hashlock, "bcache_hash");     //初始化,命名按照要求
  for(int i = 0; i < buckets_sum; i++) 
    initlock(&bcache.locks[i], "bcache_bucket");



  for(b = bcache.buf; b < bcache.buf+NBUF; b++)
    initsleeplock(&b->lock, "buffer");

}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int id=HASH(blockno);
  struct buf *pre, *minb = 0, *minpre;



  acquire(&bcache.locks[id]);

  // Is the block already cached?
  for(b = bcache.buckets[id].next; b ; b = b->next)
  {
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.locks[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  acquire(&bcache.lock);
  if(bcache.size < NBUF) {
    b = &bcache.buf[bcache.size++];
    release(&bcache.lock);
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    b->next = bcache.buckets[id].next;
    bcache.buckets[id].next = b;
    release(&bcache.locks[id]);
    acquiresleep(&b->lock);
    return b;
  }
  release(&bcache.lock);
  release(&bcache.locks[id]);  //检查是否有buf空闲


  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  uint mintime;
  acquire(&bcache.hashlock);
  for(int i = 0; i < buckets_sum; ++i) {
      mintime = -1;
      acquire(&bcache.locks[id]);
      for(pre = &bcache.buckets[id], b = pre->next; b; pre = b, b = b->next) {
          // research the block
          if(id == HASH(blockno) && b->dev == dev && b->blockno == blockno){
              b->refcnt++;
              release(&bcache.locks[id]);
              release(&bcache.hashlock);
              acquiresleep(&b->lock);
              return b;
          }
          if(b->refcnt == 0 && b->time < mintime) {
              minb = b;
              minpre = pre;
              mintime = b->time;
          }
      }
      // find an unused block
      if(minb) {
          minb->dev = dev;
          minb->blockno = blockno;
          minb->valid = 0;
          minb->refcnt = 1;
          // if block in another bucket, we should move it to correct bucket
          if(id != HASH(blockno)) {
              minpre->next = minb->next;    // remove block
              release(&bcache.locks[id]);
              id = HASH(blockno);  // the correct bucket index
              acquire(&bcache.locks[id]);
              minb->next = bcache.buckets[id].next;    // move block to correct bucket
              bcache.buckets[id].next = minb;
          }
          release(&bcache.locks[id]);
          release(&bcache.hashlock);
          acquiresleep(&minb->lock);
          return minb;
      }
      release(&bcache.locks[id]);
      if(++id == buckets_sum) {
          id = 0;
      }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int id=HASH(b->blockno);
  acquire(&bcache.locks[id]);
  b->refcnt--;
  if(b->refcnt==0)
  b->time=ticks;

  release(&bcache.locks[id]);
}

void
bpin(struct buf *b) {
  int id=HASH(b->blockno);
  acquire(&bcache.locks[id]);  //修改成对应桶的锁
  b->refcnt++;
  release(&bcache.locks[id]);
}

void
bunpin(struct buf *b) {
  int id=HASH(b->blockno);
  acquire(&bcache.locks[id]);    //修改成对应桶的锁
  b->refcnt--;
  release(&bcache.locks[id]);
}


