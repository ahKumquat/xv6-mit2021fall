#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

int matchhere(char*, char*);
int matchstar(int, char*, char*);

int match(char *re, char *text) {

  if(re[0] == '^') return matchhere(re+1, text);
  
  do { //must look at empty string
    if(matchhere(re, text)) return 1;
  } while (*text++ != '\0');
    
  return 0;
}

// matchhere: search for re at beginning of text
int matchhere(char *re, char *text) {

  if(re[0] == '\0') return 1;

  if(re[1] == '*') return matchstar(re[0], re+2, text);

  if(re[0] == '$' && re[1] == '\0') return *text == '\0';

  if(*text != '\0' && (re[0] =='.' || re[0] == *text)) return matchhere(re+1, text+1);
  
  return 0;
}

//matchstar: search for c*re at beginning of text
int matchstar(int c, char *re, char *text) {
  
  do { // a * matches zero or more instances
    if(matchhere(re, text)) return 1;
  } while(*text != '\0' && (*text++ == c || c == '.'));
  
  return 0;
}

void find(char *path, char *file) {
  char buf[512], *p;
  int fd;
  struct stat st;
  struct dirent de;

  if((fd = open(path, 0)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type) {
    case T_FILE:
      if(match(file, path)) {
        printf("%s\n", path);
      }
      break;
  
    case T_DIR:
      if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
        printf("find: path too long\n");
        break;
      }

      strcpy(buf, path);
      p = buf + strlen(buf);
      *p++ = '/';
      while (read(fd, &de, sizeof(de)) == sizeof de) {
        if(de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) continue;

        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;

        if(stat(buf, &st) < 0) {
          printf("find: cannot stat %s\n", buf);
          continue;
        }
        
        //Don't recurse into "." and ".." 
        if(strcmp(buf + strlen(buf) - 2, "./") && strcmp(buf + strlen(buf) - 3, "/..")) {
            find(buf, file);
        } 
      }
      break;
    }
    close(fd); 
}



int main(int argc, char *argv[]) {

  if(argc < 3) {
    fprintf(2,"Usage: find <dir> <filename>\n");
    exit(1);
  }

  char *path = argv[1];
  char *file = argv[2];
  
  find(path, file);

  exit(0);

}
