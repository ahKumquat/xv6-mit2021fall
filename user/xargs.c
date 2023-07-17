#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int readline(char *new_argv[MAXARG], int curr_argv) {
  char buf[1024];
  int n = 0;
  while(read(0, buf+n, 1)) {
    if(n == 1023) {
      fprintf(2, "argument is too long\n");
      exit(1);
    }
    if(buf[n] == '\n') break;
    n++;
  }

  buf[n] = 0;
  if (n == 0) return n;
  int offset = 0;

  while (offset < n) {
    new_argv[curr_argv++] = buf + offset;

    while(buf[offset] != ' ' && offset < n) offset++;

    while(buf[offset] == ' ' && offset < n) buf[offset++] = 0;

  }
  return curr_argv;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(2, "Usage: xargs <command> ...\n");
    exit(1);
  }

  char *cmd = malloc(strlen(argv[1]) + 1);
  char *new_argv[MAXARG];

  strcpy(cmd, argv[1]);

  int i;

  for(i = 1; i < argc; i++) {
    new_argv[i - 1] = malloc(strlen(argv[i]) + 1);
    strcpy(new_argv[i - 1], argv[i]);
  }
  
  int curr_argv;
  while((curr_argv = readline(new_argv, argc - 1))) {
    new_argv[curr_argv] = 0;
    if(!fork()) {
      exec(cmd, new_argv);
      fprintf(2, "exec failed\n");
      exit(1);
    }
    wait(0);    
  }  
  exit(0);
}
