struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  struct buf *next;
  uchar data[BSIZE];
  uint  time; // instead time-stamp buffers using the time of their last use.
};

