//jobsched.c
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
#include <pthread.h>

struct job *head = NULL;
struct list *jobs_queue;
struct list *all_jobs;
int N_JOB = 1;
pthread_mutex_t lock;
int WRKNG_JOBS = 0;
pthread_cond_t shell_wait = PTHREAD_COND_INITIALIZER;
int W8_JOB = 0;

struct job
{
  int job_id;
  char state;
  char exit;
  char* command;
  char** argv;
  struct job *next;
};
struct list{
  struct job *head;
};

// void set_job_status(int job_id, struct list *l, char*){
  // while(l->head->job){
  //
  // }
// }

struct job* job_create(int job_id, char* command, char* argv[]){

  struct job *j = 0;
  j = malloc(sizeof(struct job));
  j->job_id = job_id;
  j->state = '0';
  j->exit = '-';
  j->command = command;
  j->argv = argv;
  return j;
}

void job_dequeue(struct list *l){
  l->head = l->head->next;
}

void queue_job(struct list *l, struct job *j){
  //linking the jobs
  j->next = l->head;
  //by assigning the head to the foremost task, it helps the next created task to point to this one
  l->head = j;
}

void* shell_thread_func(){

  char line[1000];
  char *words[100];
  char *word;
  int nwords = 0;
  int job_counter = 1;
  struct job* j;
  printf("> ");

  while((fgets(line, 1000, stdin)) != NULL){

    word = strtok(line, " \t\n");
    while (word != NULL)
    {
      words[nwords] = word;
      word = strtok(NULL, " \t\n");
      nwords++;
    }
    if(nwords == 0){
      printf("> ");
      fflush(stdout);
      continue;
    }
    words[nwords] = NULL;

    //1. Submit
    /* The submit command defines a new job and the Unix command that should be
    run when the job is scheduled. It should return immediately and display a
    unique integer job ID generated internally by your program.
    (Just start at one and count up.)
    The job will then run in the background when selected by the scheduler.*/
    if(strcmp(words[0], "submit") == 0){
      printf("submit entered. \n");
      printf("%s\n", words[1]);

      pthread_mutex_lock(&lock);
      j = job_create(job_counter, words[1], words + 1);
      queue_job(jobs_queue, j);
      queue_job(all_jobs, j);
      printf("job %d is queued\n", job_counter);
      job_counter++;
      pthread_mutex_unlock(&lock);

      //2. Status
      /* 2. The status command lists all of the jobs currently known, giving the
      job id, current state (waiting, running, or done) the exit status of the job
      (if done) and the Unix command of the job */
    } else if(strcmp(words[0], "status") == 0){

      //TODO:

      printf("status entered. \n");
      if(nwords == 0){
        printf("JOBID   STATE   EXIT COMMAND");
        // struct task *a = all_jobs->head;
        // while(a != NULL){
        //   if(a->state == )
        //   printf("%d     %s     %d")
        //   a = a->next;
        // }
      } else {
        fprintf(stderr, "Error: Too many arguments. Usage: $ submit\n");
      }
      //3. Wait
      /* takes a jobid and pauses until that job is done executing. When the job
      is complete, it should display the job ID and command, and then display the
      standard output generated by the job. (If the job was already complete, or
      wait is called multiple times, it should just display the relevant
      information right away.) */
    } else if(strcmp(words[0], "wait") == 0){
      printf("wait entered. \n");
      int job_id;
      if(nwords == 2){
        job_id = atoi(words[1]);
        printf("waiting for job: %d \n",job_id);
        W8_JOB = job_id;
        pthread_cond_wait(&shell_wait, &lock);
        //standard output
        printf("Wait job %d is finished\n",job_id);
        W8_JOB = 0;

      } else {
        fprintf(stderr, "Error: Wrong number of arguments. Usage: $ wait <n>\n");
      }

      //4. remove
      /* takes a jobid and then removes it from the queue, also deleting any
      stored output of the job. However, it should only do this if the job is in
      the WAIT or DONE states. If the job is currently running, this command
      should display a suitable error, and refuse to remove the job*/
    } else if(strcmp(words[0], "remove") == 0){

      if(nwords == 2){
        printf("%d",nwords);
      } else {
        fprintf(stderr, "Error: Wrong number of arguments. Usage: $ njobs <n>\n");
      }

      printf("remove entered. \n");

      //5. njobs
      /*The njobs command indicates how many jobs the scheduler may run at once,
      which should be one by default. */
    } else if(strcmp(words[0], "njobs") == 0){
      printf("njobs entered. \n");
      if(nwords == 2){
        N_JOB = atoi(words[1]);
      } else {
        fprintf(stderr, "Wrong number of arguments. \nUsage: $ njobs <n>\n");
      }

      //6. Drain
      /*wait until all jobs in the queue are in the DONE state. */
    } else if(strcmp(words[0], "drain") == 0){
      printf("drain entered. \n");
      if(nwords == 1){
        while(jobs_queue->head != NULL)//job queue is not empty
          sleep(1);
      } else {
        fprintf(stderr, "Error: Too many arguments. Usage: $ drain\n");
      }

      //7. Quit
      /*immediately exit the program, regardless of any jobs in the queue.
      (If end-of-file is detected on the input,
      the program should quit in the same way.)*/
    } else if(strcmp(words[0], "quit") == 0){
      printf("quit entered. \n");


    } else if(strcmp(words[0], "help") == 0){
      printf("help entered. \n");
    }
    nwords = 0;
    printf("> ");
    fflush(stdout);
  }
  return 0;

}

void* sched_thread_func(){
  while(1){ //condition variable

    if(jobs_queue->head != NULL){

      pthread_mutex_lock(&lock);
      struct job *j1 = jobs_queue->head;
      job_dequeue(jobs_queue);

      printf("J1 %s\n", j1->command);

      //set the j1->job_id state to running in all_jobs
      WRKNG_JOBS++;
      pthread_mutex_unlock(&lock);

      //Sched Thread Fork() and Exec()
      int status = EXIT_SUCCESS;
      pid_t  pid = fork();
      if (pid < 0) {	    // Fork Error
        fprintf(stderr, "myshell: Unable to fork: %s\n", strerror(errno));
      }
      if (pid == 0) {	    // Child

        printf("process %d started\n", getpid());
        if (execvp(j1->command, j1->argv)) {

          fprintf(stderr, "Unable to execvp: %s %s\n", j1->command, strerror(errno));
          _exit(EXIT_FAILURE);
        }
      }
      if (pid > 0){
        if(WRKNG_JOBS < N_JOB){
          pid = waitpid(-1, &status, WNOHANG); //Non blocking wait
          // printf("Job %d finished \n", j1->job_id);
        } else {
          pid = wait(&status); //Blocking wait

        }
        if(pid != 0){
          printf("Job %d finished \n", j1->job_id);
          if(j1->job_id == W8_JOB){
            pthread_cond_broadcast(&shell_wait);
          }
            //TODO: change the status of j1->job_id in all_jobs, change its exit status
        }
      }
    }
  }
}

int main( int argc, char *argv[] )
{

  if (pthread_mutex_init(&lock, NULL) != 0)
  {
        printf("\n mutex init has failed\n");
        return 1;
  }
  jobs_queue = malloc(sizeof(struct list));
  all_jobs = malloc(sizeof(struct list));
  jobs_queue->head = head;
  all_jobs->head = head;

  pthread_t shell_thread;
  pthread_t sched_thread;
  int a;
  int b;
  a = pthread_create(&shell_thread, NULL, shell_thread_func, NULL);
  b = pthread_create(&sched_thread, NULL, sched_thread_func, NULL);

  pthread_join(shell_thread, NULL);

  return EXIT_SUCCESS;
}
