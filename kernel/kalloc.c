// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define PA2PGREF_IDX(p) ((p - KERNBASE) / PGSIZE)
#define PGREF_MAX_ENTRIES PA2PGREF_IDX(PHYSTOP)
#define PA2REF(p) pageref[PA2PGREF_IDX((uint64)p)]

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct spinlock pgreflock;
int pageref[PGREF_MAX_ENTRIES];

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&pgreflock, "pgreflock");
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


  acquire(&pgreflock);

  if (--PA2REF(pa) <= 0) // if reference count is less or equal to 0, free the pages
  {
   
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }

  release(&pgreflock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk

    PA2REF(r) = 1; //Set a page's reference count to one when kalloc() allocates it
  }
  return (void*)r;
}


//increment a page's reference count when fork causes a child to share the page, 
//and decrement a page's count each time any process drops the page from its page table.

void increasepa(void *pa){
  acquire(&pgreflock);
  PA2REF(pa)++;
  release(&pgreflock);
}


void *kcopy_deref (void *pa) {
  acquire(&pgreflock);

  // only one reference
  if(PA2REF(pa) <= 1) {
    release(&pgreflock);
    return pa;
  }

  uint64 newpa = (uint64) kalloc();

  // out of memory
  if (newpa == 0) {
    release(&pgreflock);
    return 0; 
  }

  // decrement a page's count each time any process drops the page from its page table.
  memmove((void*)newpa, (void*)pa, PGSIZE);
  PA2REF(pa)--;

  release(&pgreflock);

  return (void*)newpa;
}