#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <inttypes.h>

#define streq(s0, s1) (strcmp((s0), (s1)) == 0)
typedef enum { WAIT=0, RUN, DONE } State;

typedef struct Job {
	int id;
	pid_t pid;
	struct Job* next;
	State state;
	int exit;
	char* commands;
	char** words;
} Job;

typedef struct list {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	Job* head;
	int njobs;
} list;

list* job_queue_create(){
	list* job_queue = malloc(sizeof(list));
	job_queue->head = NULL;
	job_queue->njobs = 1;
	pthread_mutex_init(&job_queue->mutex, NULL);
	pthread_cond_init(&job_queue->cond, NULL);
	return job_queue;
}


Job* create_job(char* commands, int nwords, char* words[]){
	// id
	static int jobid = 1;
	Job* job = malloc(sizeof(Job));
	job->id = jobid++;
	job->words = malloc(nwords*sizeof(char*));
	int i;
	for (i = 0; words[i+1] != NULL; i++){
		job->words[i] = strdup(words[i+1]);
	}
	job->words[i] = NULL; // null terminated
	job->commands = malloc(sizeof(commands));
	strcpy(job->commands, commands);
	job->next = NULL;
	job->state = WAIT;
	job->exit = -1;
	job->pid = -1;
	return job;
}

//adding a job to the queue
int queue_job(list* job_queue, Job* job){
	pthread_mutex_lock(&job_queue->mutex);

	if (job_queue->head == NULL){
		job_queue->head = job;
		pthread_mutex_unlock(&job_queue->mutex);
		return EXIT_SUCCESS;
	}
	Job* j = job_queue->head;
	while(j->next != NULL){
		j = j->next;
	}
	j->next = job;
	pthread_mutex_unlock(&job_queue->mutex);
	return EXIT_SUCCESS;
}

void set_njobs(list* job_queue, int njobs){
	pthread_mutex_lock(&job_queue->mutex);
	if (njobs > 0){
		job_queue->njobs = njobs;
		pthread_cond_broadcast(&job_queue->cond);
	}
	pthread_mutex_unlock(&job_queue->mutex);
}


int run_job(Job* wait_jobJob){
	wait_jobJob->state = RUN;
	pid_t pid = fork();
	if (pid < 0){
		fprintf(stderr, "Error: Unable to fork: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}	else if (pid == 0){

		struct stat st = {0};
		if (stat("./outputs", &st) == -1){
			mkdir("./outputs", 0777);
		}
		char outputFileName[20];
		sprintf(outputFileName, "./outputs/output.%d", wait_jobJob->id);
		int output_dir = open(outputFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG);
		dup2(output_dir, 1);
		dup2(output_dir, 2);
		close(output_dir);
		execvp(wait_jobJob->words[0], wait_jobJob->words);
		fprintf(stderr, "Unable to execvp: %s\n", strerror(errno));
		exit(1);
	} else {
		wait_jobJob->pid = pid;
	}
	return EXIT_SUCCESS;
}

int job_complete(list* job_queue, pid_t pid, int exitStatus){
	pthread_mutex_lock(&job_queue->mutex);
	Job* curr = job_queue->head;
	while (curr != NULL){

		if (curr->pid == pid){
			curr->state = DONE;
			curr->exit = exitStatus;
			pthread_cond_broadcast(&job_queue->cond);
			pthread_mutex_unlock(&job_queue->mutex);
			return EXIT_SUCCESS;
		}
		curr = curr->next;
	}
	pthread_mutex_unlock(&job_queue->mutex);
	return 2;
}
int numJobsRunning(list* job_queue){
	int jobs_running = 0;
	Job* job = job_queue->head;
	while (job != NULL){
		if (job->state == RUN){
			jobs_running = jobs_running + 1;
		}
		job = job->next;
	}
	return jobs_running;
}
int select_job_to_run(list* job_queue){
	pthread_mutex_lock(&job_queue->mutex);
	int returnVal = -1;

	if (job_queue->head == NULL){
		returnVal = 1;
	} else {
		while (numJobsRunning(job_queue) >= job_queue->njobs){
			pthread_cond_wait(&job_queue->cond, &job_queue->mutex);
		}
		Job* wait_jobJob = job_queue->head;
		while (wait_jobJob->state != WAIT){
			wait_jobJob = wait_jobJob->next;
			if (wait_jobJob == NULL){
				returnVal = 1;
				break;
			}
		}
		if (wait_jobJob != NULL){
			returnVal = run_job(wait_jobJob);
		}
	}

	pthread_mutex_unlock(&job_queue->mutex);
	return returnVal;
}

//function to delete job
int remove_job(list* job_queue, int id){
	pthread_mutex_lock(&job_queue->mutex);

	if (job_queue->head == NULL){

		pthread_mutex_unlock(&job_queue->mutex);
		return 1; // empty
	} else if (job_queue->head->id == id && job_queue->head->state != RUN){
		job_queue->head = job_queue->head->next;

		char outputFileName[20];
		sprintf(outputFileName, "./output.%d", id);
		remove(outputFileName);
		pthread_mutex_unlock(&job_queue->mutex);
		return EXIT_SUCCESS;
	}

	Job* wait_jobJob = job_queue->head->next;
	Job* prevJob     = job_queue->head;
	while (prevJob->next != NULL && wait_jobJob->id != id){
		wait_jobJob = wait_jobJob->next;
		prevJob     = prevJob->next;
	}
	if (prevJob->next == NULL){

		pthread_mutex_unlock(&job_queue->mutex);
		return 1;
	} else if (wait_jobJob->state == RUN){

		pthread_mutex_unlock(&job_queue->mutex);
		return -1;
	} else {
		char outputFileName[20];
		sprintf(outputFileName, "./output.%d", id);
		remove(outputFileName);
		prevJob->next = wait_jobJob->next;
		pthread_mutex_unlock(&job_queue->mutex);
		return EXIT_SUCCESS;
	}
}
