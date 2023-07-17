#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void exec_prime(int fd[2]) {
  int num;
  int tmp = -1;
  int p[2];

  read(fd[0], &num, sizeof(num));

  printf("prime %d\n",num);
  pipe(p);

  while(read(fd[0], &tmp, sizeof(tmp))) {
      if(tmp % num != 0) {
        write(p[1], &tmp, sizeof(tmp));
      }
  }

  if(tmp == -1) {
    close(p[1]);
    close(p[0]);
    close(fd[0]);
    return;
  }
  
  if(fork() == 0) {
    close(p[1]);
    close(fd[0]);
    exec_prime(p);
    close(p[0]);
  } else {
    close(p[1]);
    close(p[0]);
    close(fd[0]);
    wait(0);
  }
}



void main() {

  int i;
  int p[2];
  pipe(p);

  for (i = 2; i < 35 ; i++) {
      write(p[1], &i, sizeof(i));
  }

  close(p[1]);
  
  exec_prime(p);
  
  close(p[0]);

  exit(0);

 }

