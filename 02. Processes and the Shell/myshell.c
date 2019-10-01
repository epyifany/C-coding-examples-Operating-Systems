//myshell.c
//Yifan Yu

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

void handler(int signum) {
	// puts("Handler");
}

int main(int argc, char *argv[]) {
  char line[1000];
  char *words[100];
  char *word;
  int nwords = 0;

  printf("myshell> ");
  fflush(stdout);

  while((fgets(line, 1000, stdin)) != NULL){
    // printf("myshell> ");
    word = strtok(line, " \t\n");
    while (word != NULL)
    {
      words[nwords] = word;
      word = strtok(NULL, " \t\n");
      nwords++;
    }
    if(nwords == 0){
      printf("myshell> ");
      fflush(stdout);
      continue;
    }
    words[nwords] = NULL;
    // 1. start
    if(strcmp(words[0], "start") == 0){
      pid_t  pid = fork();
      if (pid < 0) {	    // Fork Error
        fprintf(stderr, "myshell: Unable to fork: %s\n", strerror(errno));
      }
      if (pid == 0) {	    // Child
        printf("myshell: ");
        printf("process %d started\n", getpid());
        if (execvp(words[1], words + 1)) {
          fprintf(stderr, "myshell: Unable to execvp: %s\n", strerror(errno));
          _exit(EXIT_FAILURE);
        }
      }
      // 2. wait
    } else if(strcmp(words[0], "wait") == 0){
      pid_t  pid;
      int status = EXIT_SUCCESS;
      pid = wait(&status);
      if(pid < 0){
        if(errno == ECHILD){
          fprintf(stderr, "myshell: No children.\n");
        }
      } else if (status != 0){
        fprintf(stderr, "myshell: process %d exited abnormally with signal %d: %s.\n", pid, WEXITSTATUS(status), strerror(errno));
      } else {
        printf("process %d exited normally with status %d\n", pid, WEXITSTATUS(status));
      }
      // 3. waitfor
    } else if(strcmp(words[0], "waitfor") == 0){

      pid_t  pid;
      pid_t check_pid = atoi(words[1]);
      int status = EXIT_SUCCESS;
      pid = waitpid(check_pid, &status, 0);
      if(pid < 0){
        if(errno == ECHILD){
          fprintf(stderr, "myshell: No such process.\n");
        }
      } else if (status != 0){
        fprintf(stderr, "myshell: process %d exited abnormally with signal %d: %s.\n", pid, WEXITSTATUS(status), strerror(errno));
      } else {
        printf("process %d exited normally with status %d\n", pid, WEXITSTATUS(status));
      }
      // 4. run
    } else if(strcmp(words[0], "run") == 0){
      int status = EXIT_SUCCESS;
      pid_t  pid = fork();

      if (pid < 0) {	    //Error
        fprintf(stderr, "myshell: Unable to fork: %s\n", strerror(errno));
      }
      if (pid == 0) {	    // Child
        printf("myshell: ");
        printf("process %d started\n", getpid());
        if (execvp(words[1], &words[1])) {
          fprintf(stderr, "myshell: Unable to execvp: %s\n", strerror(errno));
          _exit(EXIT_FAILURE);
        }
      } else {		    // Parent
        // printf("parent of run\n");
        while (waitpid(pid, &status, 0) < 0);
        if (status != 0){
          fprintf(stderr, "myshell: process %d exited abnormally with signal %d: %s.\n", pid, WEXITSTATUS(status), strerror(errno));
        } else {
          printf("myshell: process %d exited normally with status %d\n", pid, WEXITSTATUS(status));
        }
      }
    // 5. Watchdog
    } else if(strcmp(words[0], "watchdog") == 0){
      printf("watchdog entered. \n");
      int status = EXIT_SUCCESS;
      int watchtime = atoi(words[1]);
      pid_t  pid = fork();
      int kill_issued = NULL;

      if (pid < 0) {	    //Error
        fprintf(stderr, "myshell: Unable to fork: %s\n", strerror(errno));
      }
      if (pid == 0) {	    // Child
        printf("myshell: ");
        printf("process %d started\n", getpid());
        if (execvp(words[2], &words[2])) {
          fprintf(stderr, "myshell: Unable to execvp: %s\n", strerror(errno));
          _exit(EXIT_FAILURE);
        }
      } else {		    // Parent
        signal(SIGCHLD, handler);
        if(sleep(watchtime) == 0){
          kill(pid, SIGKILL);
          printf("myshell: process %d exceeded the time limit, killing it...\n", pid);
          kill_issued = 1;
        }

        while (waitpid(pid, &status, 0) < 0);

        if (kill_issued){
          printf("myshell: process %d exited abnormally with signal %d: %s\n", pid, WTERMSIG(status), strsignal(WTERMSIG(status)));
        }
        else if (status != 0){
          fprintf(stderr, "myshell: process %d exited abnormally with signal %d: %s.\n", pid, WEXITSTATUS(status), strerror(errno));
        } else {
          printf("myshell: process %d exited normally with status %d\n", pid, WEXITSTATUS(status));
        }
      }

    //6. unknown command
    } else {
      fprintf(stderr, "myshell: unknown command: %s\n", words[0]);
    }

    nwords = 0;
    printf("myshell> ");
    fflush(stdout);

  }

  return EXIT_SUCCESS;
}

/* vim: set sts=4 sw=4 ts=8 expandtab ft=c: */
