// Tests to drive abstract sharing analysis

#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "mtrace.h"
#include "pthread.h"

#include <sys/mman.h>

static int cpu;
static pthread_barrier_t bar;
enum { ncore = 8 };

void
next()
{
  if (setaffinity(cpu) < 0) {
    cpu = 0;
    if (setaffinity(cpu) < 0)
      die("sys_setaffinity(%d) failed", cpu);
  }
  cpu++;
}

void*
vmsharing(void* arg)
{
  u64 i = (u64) arg;

  volatile char *p = (char*)(0x40000UL + i * 4096);
  if (mmap((void *) p, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) < 0)
    die("mmap failed");

  if (munmap((void *) p, 4096) < 0)
    die("munmap failed");

  return 0;
}

void*
fssharing(void* arg)
{
  u64 i = (u64) arg;

  // Note that we keep these files open; otherwise all of these
  // operations will share the abstract FD object and we won't get any
  // results.

  char filename[32];
  snprintf(filename, sizeof(filename), "f%d", i);

  open(filename, O_CREATE|O_RDWR);

  pthread_barrier_wait(&bar);

  for (u64 j = 0; j < ncore; j++) {
    snprintf(filename, sizeof(filename), "f%d", j);
    open(filename, O_RDWR);
  }
  return 0;
}

int
main(int ac, char **av)
{
  void* (*op)(void*) = 0;
  if (ac == 2 && strcmp(av[1], "vm") == 0)
    op = vmsharing;
  else if (ac == 2 && strcmp(av[1], "fs") == 0)
    op = fssharing;
  else
    fprintf(1, "usage: %s vm|fs\n", av[0]);

  if (op) {
    mtenable_type(mtrace_record_ascope, "xv6-asharing");
    pthread_barrier_init(&bar, 0, ncore);
    for (u64 i = 0; i < ncore; i++) {
      next();
      pthread_t tid;
      pthread_create(&tid, 0, op, (void*) i);
    }

    for (u64 i = 0; i < ncore; i++)
      wait();
    mtdisable("xv6-asharing");
  }
}
