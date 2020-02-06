struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE } type;  // file的类型
  int ref;            // reference count
  char readable;      // 是否可读
  char writable;      // 是否可写
  struct pipe *pipe;  // pipe pointer
  struct inode *ip;   // inode pointer
  uint off;           // 读写到的偏移量，用于inode的读写
};


// in-memory copy of an inode
struct inode {
  uint dev;           // Device number
  uint inum;          // Inode number
  int ref;            // Reference count
  struct sleeplock lock; // protects everything below here
  int valid;          // inode has been read from disk?

  short type;         // copy of disk inode
  short major;
  short minor;
  short nlink;
  uint size;          // 关联的block的byte数
  uint addrs[NDIRECT+1];  // 关联的block num
};

// table mapping major device number to
// device functions
struct devsw {
  int (*read)(struct inode*, char*, int);
  int (*write)(struct inode*, char*, int);
};

extern struct devsw devsw[];

#define CONSOLE 1
