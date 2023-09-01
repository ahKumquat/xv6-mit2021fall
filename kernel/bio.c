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

#define NBUCKET 13

uint extern ticks;

struct {
  struct spinlock lock[NBUCKET];
  struct buf buf[NBUF];

  struct buf head[NBUCKET];
} bcache;

int lockable(int id, int idx) {
  int n = NBUCKET / 2;
  if (id <= n) {
      if (id < idx && idx <= (id + n)) return 0;
  } else {
    if ((id < idx && idx < NBUCKET) || (idx <= (id + n) % NBUCKET)) return 0;
  }
  return 1;
}

void
binit(void)
{
  struct buf *b;

  for (int i = 0 ; i < NBUCKET; i++)
    initlock(&bcache.lock[i], "bcache");

  bcache.head[0].next = &bcache.buf[0];
  // append all bufferss to bucket 0
  for(b = bcache.buf; b < bcache.buf+NBUF - 1; b++){
    b->next = b+1;  
    initsleeplock(&b->lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  struct buf *res;

  int id = blockno % NBUCKET;

  acquire(&bcache.lock[id]);

  b = bcache.head[id].next;

  // Is the block already cached?
  while (b)
  {
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[id]);
      acquiresleep(&b->lock);
      return b;
    }
    b = b -> next;
  }

  int idx = -1;
  uint smallest_tick = __UINT32_MAX__;
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(int i = 0; i < NBUCKET; i++){
    if(i != id && lockable(id, i)) {
      acquire(&bcache.lock[i]);
    } else if (!lockable(id, i)) {
      continue;
    }
    b = bcache.head[i].next;
    while (b) {
      if (b -> refcnt == 0) {
        if (smallest_tick > b -> time){
          smallest_tick = b -> time;

          // if this smallest tick is not in the last smallest tick's hashtable, release the corresponding hashtable'lock of last smallest tick.
          if (idx != -1 && idx != i && holding(&bcache.lock[idx])) release(&bcache.lock[idx]);
          idx = i;
        }
      }
      b = b -> next;
    }
    if (i != id && i != idx && holding(&bcache.lock[i])) release(&bcache.lock[i]);
  }

  if (idx == -1) panic("bget: no buffers");

  b = &bcache.head[idx];

  // when it finds the LRU
  while (b)
  {
    if ((b -> next) -> refcnt == 0 && (b -> next) -> time == smallest_tick) {
        res = b -> next;
        b -> next = b -> next -> next;
        break;
    }
    b = b -> next;
  }
  
  if (idx != id && holding(&bcache.lock[idx])) release(&bcache.lock[idx]);

  b = &bcache.head[id];
  while (b -> next)
  {
    b = b -> next;
  }
  b -> next = res;
  res -> next = 0;
  res -> dev = dev;
  res -> blockno = blockno;
  res -> valid = 0;
  res -> refcnt = 1;
  
  release(&bcache.lock[id]);
  
  acquiresleep(&res -> lock);
  return res;
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

  int id = b -> blockno % NBUCKET;
  
  acquire(&bcache.lock[id]);

  b->refcnt--;

  if (b->refcnt == 0) 
    b -> time = ticks;
  
  release(&bcache.lock[id]);
}

void
bpin(struct buf *b) {
  int id = b -> blockno % NBUCKET;
  acquire(&bcache.lock[id]);
  b->refcnt++;
  release(&bcache.lock[id]);
}

void
bunpin(struct buf *b) {
  int id = b -> blockno % NBUCKET;
  acquire(&bcache.lock[id]);
  b->refcnt--;
  release(&bcache.lock[id]);
}


