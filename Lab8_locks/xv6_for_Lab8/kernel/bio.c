// Buffer cache.
//
// The buffer cache is a hash table of buf structures holding
// cached copies of disk block contents: buffers are grouped into
// NBUCKET doubly-linked lists, one list (and one lock) per bucket.
// Caching disk blocks in memory reduces the number of disk reads
// and also provides a synchronization point for disk blocks used by
// multiple processes.
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

#define NBUCKET 13  // number of hash buckets; a prime reduces conflicts

struct {
  struct spinlock lock;      // serializes the search for a free buffer in bget
  struct buf buf[NBUF];

  // Hash table of all buffers, through prev/next.
  // bucket[i] is a doubly-linked list of buffers that hash to bucket i.
  struct buf bucket[NBUCKET];
  struct spinlock bucket_locks[NBUCKET];
} bcache;

// Which bucket holds the given block?
static int
bhash(uint blockno)
{
  return blockno % NBUCKET;
}

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  for(int i = 0; i < NBUCKET; i++){
    initlock(&bcache.bucket_locks[i], "bcache.bucket");
    bcache.bucket[i].prev = &bcache.bucket[i];
    bcache.bucket[i].next = &bcache.bucket[i];
  }

  // Create linked list of buffers; start with all of them in bucket 0.
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.bucket[0].next;
    b->prev = &bcache.bucket[0];
    initsleeplock(&b->lock, "buffer");
    bcache.bucket[0].next->prev = b;
    bcache.bucket[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bkt = bhash(blockno);

  // Is the block already cached?  Take only the bucket's lock, so
  // lookups of blocks in different buckets run in parallel.
  acquire(&bcache.bucket_locks[bkt]);
  for(b = bcache.bucket[bkt].next; b != &bcache.bucket[bkt]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucket_locks[bkt]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bucket_locks[bkt]);

  // Not cached.  Serialize the search for a free buffer with
  // bcache.lock; this part of bget is allowed to be a bottleneck.
  acquire(&bcache.lock);

  // The bucket lock was released above, so someone else may have
  // cached this block in the meantime; re-check under the bucket lock
  // to keep the invariant that at most one copy of each block exists.
  acquire(&bcache.bucket_locks[bkt]);
  for(b = bcache.bucket[bkt].next; b != &bcache.bucket[bkt]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucket_locks[bkt]);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bucket_locks[bkt]);

  // Recycle any unused buffer (refcnt == 0); no LRU is needed.
  // While we hold bcache.lock no other CPU performs this search, so
  // b->blockno (and therefore bhash(b->blockno)) cannot change here.
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    int oldbkt = bhash(b->blockno);
    acquire(&bcache.bucket_locks[oldbkt]);
    if(b->refcnt == 0){
      // Remove b from its old bucket.
      b->next->prev = b->prev;
      b->prev->next = b->next;
      release(&bcache.bucket_locks[oldbkt]);

      // Give b the new block's identity and link it into bucket bkt.
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      acquire(&bcache.bucket_locks[bkt]);
      b->next = bcache.bucket[bkt].next;
      b->prev = &bcache.bucket[bkt];
      bcache.bucket[bkt].next->prev = b;
      bcache.bucket[bkt].next = b;
      release(&bcache.bucket_locks[bkt]);

      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
    release(&bcache.bucket_locks[oldbkt]);
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
// There is no LRU list to update, so brelse only releases the sleep
// lock and decrements refcnt under the bucket's lock; no global
// bcache.lock is needed.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int bkt = bhash(b->blockno);
  acquire(&bcache.bucket_locks[bkt]);
  b->refcnt--;
  release(&bcache.bucket_locks[bkt]);
}

void
bpin(struct buf *b) {
  int bkt = bhash(b->blockno);
  acquire(&bcache.bucket_locks[bkt]);
  b->refcnt++;
  release(&bcache.bucket_locks[bkt]);
}

void
bunpin(struct buf *b) {
  int bkt = bhash(b->blockno);
  acquire(&bcache.bucket_locks[bkt]);
  b->refcnt--;
  release(&bcache.bucket_locks[bkt]);
}


