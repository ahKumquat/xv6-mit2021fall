#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void main() {
  char buf[100];
  int fds[2];
  pipe(fds);
  int pid = fork();
  if(pid == 0) {
    write(fds[1], buf, 1);
    printf("%d: received ping\n", pid);
    close(fds[1]);
    exit(0);
  } else {
    wait(0);
    read(fds[0], buf, 1);
    printf("%d: received pong\n", pid);
    close(fds[0]);
    exit(0);
  }
}
