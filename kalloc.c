// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, pgae table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"
#include "stdio.h"
struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
  uint pgrefcount[PHYSTOP>>PGSHIFT];
} kmem;

void dec_refcounter(uint pa)
{
  acquire(&kmem.lock);
  --kmem.pgrefcount[pa >> PGSHIFT];
  release(&kmem.lock);
}

void inc_refcounter(uint pa)
{
  acquire(&kmem.lock);
  ++kmem.pgrefcount[pa >> PGSHIFT];
  release(&kmem.lock);
}
uint get_refcounter(uint pa)
{
  uint count;

  acquire(&kmem.lock);
  count = kmem.pgrefcount[pa >> PGSHIFT];
  release(&kmem.lock);

  return count;
}

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
                   // defined by the kernel linker script in kernel.ld
int numfreepages = 0;

struct run {
  struct run *next;
};

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE){
    kmem.pgrefcount[V2P(p) >> PGSHIFT] = 0;          // initialse the reference count to 0
    kfree(p);
  }
}
//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = (struct run*)v;

  if(kmem.pgrefcount[V2P(v) >> PGSHIFT] > 0)         // Decrement the reference count of a page whenever someone frees it
    --kmem.pgrefcount[V2P(v) >> PGSHIFT];

  if(kmem.pgrefcount[V2P(v) >> PGSHIFT] == 0){       // Free the page only if there are no references to the page
    // Fill with junk to catch dangling refs.
    memset(v, 1, PGSIZE);
    numfreepages++;                                // Increment the number of free pages by 1 when a page is freed
    r->next = kmem.freelist;
    kmem.freelist = r;

  }
  if(kmem.use_lock)
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    numfreepages--;                                  // Decrement the number of free pages by 1 on a page allocation
    kmem.pgrefcount[V2P((char*)r) >> PGSHIFT] = 1;     // reference count of a page is set to one when it is allocated

  }
  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)r;
}

int freemem(){
  return numfreepages;
}



