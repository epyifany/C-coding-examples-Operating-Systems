//Yifan Yu
//treecopy.c

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

int FILE_COUNT = 0, DIR_COUNT = 0, BYTE_COUNT = 0;

#define streq(a, b) (strcmp(a, b) == 0)

int filecpy(char *src, char *dst){

  /* Open source file for reading */
  int rfd = open(src, O_RDONLY); //read only
  if (rfd < 0) {
    fprintf(stderr, "treecopy: Unable to open %s: %s\n", src, strerror(errno));
    exit(1);
  }

  /* Open destination file for writing */ //create write only
  int wfd = open(dst, O_CREAT|O_WRONLY, 0644); // Add mode, no funky permissions
  if (wfd < 0) {
    fprintf(stderr, "treecopy: Unable to create %s: %s\n", dst, strerror(errno));
    exit(1);
  }

  /* Copy from source to destination */
  char buffer[BUFSIZ];
  int  nread;



  while ((nread = read(rfd, buffer, BUFSIZ)) > 0) {
      int nwritten = write(wfd, buffer, nread);
      while (nwritten != nread) {                 // Write in loop
          nwritten += write(wfd, buffer + nwritten, nread - nwritten);
      }
      BYTE_COUNT += nwritten;
      if(nwritten < 0){                           // Error writing
        fprintf(stderr, "treecopy: Unable to write to file %s: %s\n", dst, strerror(errno));
        exit(1);
      }
  }

  if(nread < 0){                                   // Error writing
    fprintf(stderr, "treecopy: Unable to read %s: %s\n", src, strerror(errno));
    exit(1);
  }

  FILE_COUNT++;
  close(rfd);
  close(wfd);
  return 0;
}

int walk(char *src, char *dst){

  //opendir source directory and makedir destination directory
  DIR *dir = opendir(src);
  if (!dir) {
    fprintf(stderr, "treecopy: Unable to open directory %s: %s\n", src, strerror(errno));
    exit(1);
  }
  int makeDir = mkdir(dst, 0777);
  if (makeDir < 0) {
    fprintf(stderr, "treecopy: Unable to make directory %s: %s\n", dst, strerror(errno));
    exit(1);
  } else {
    DIR_COUNT++;
  }

  errno = 0; //Errno to check readdir errors

  for (struct dirent *e = readdir(dir); e; e = readdir(dir)) {

    if (streq(e->d_name, "..") || streq(e->d_name, "."))
        continue;
    char srcpath[BUFSIZ];
    char dstpath[BUFSIZ];
    snprintf(srcpath, BUFSIZ, "%s/%s", src, e->d_name);
    snprintf(dstpath, BUFSIZ, "%s/%s", dst, e->d_name);

    if (e->d_type == DT_DIR){
      int walk_err = walk(srcpath, dstpath); //recursively walk through the directories
      if (walk_err){
        exit(1);
      }
    }
    else if (e->d_type == DT_REG){
      filecpy(srcpath, dstpath); //copy if it is a file
    }
  }
  if(errno){
    fprintf(stderr, "treecopy: Unable to read directory %s: %s\n", src, strerror(errno));
    exit(1);
  }

  closedir(dir);
  return 0;
}

int main(int argc, char *argv[]) {
    /* Parse command-line options */
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <SourcePath> <TargetPath>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *src = argv[1];
    char *dst = argv[2];
    struct stat s;

    //Destination Path Exists
    struct stat t;
    if ((stat(dst, &t) == 0) && S_ISDIR(t.st_mode)){
      fprintf(stderr, "treecopy: Destination Path %s Exists\n", argv[2]);
      return EXIT_FAILURE;
    }

    //Source Path Doesnt Exists
    if (stat(src, &s) != 0){
      fprintf(stderr, "treecopy: Source Path %s Doesnt Exist\n", argv[1]);
      return EXIT_FAILURE;
    }

    if (S_ISREG(s.st_mode)){ //IF THE SOURCE IS JUST A FILE
      if(filecpy(src, dst) == 0){
        return EXIT_SUCCESS;
      }
      else {
        return EXIT_FAILURE;
      }
    }
    else if(S_ISDIR(s.st_mode)){ //IF THE SOURCE IS A Directory
      if(walk(src, dst) == 0){
        printf("treecopy: copied %d directories, %d files, and %d bytes from %s to %s\n", DIR_COUNT, FILE_COUNT, BYTE_COUNT, src, dst);
        return EXIT_SUCCESS;
      }
      else {
        return EXIT_FAILURE;
      }
    }
    else{  //IF THE SOURCE IS STH ELSE
      fprintf(stderr, "treecopy: Source Path %s is not a file or a directory\n", argv[1]);
      return EXIT_FAILURE;
    }

}
