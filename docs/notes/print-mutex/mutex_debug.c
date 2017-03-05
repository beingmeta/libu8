#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
       
extern pthread_mutex_t spotify_mutex, g_notify_mutex;

#define MAX_THREADS 32
struct thread_info {
	pthread_t id;
	char *name;
} thread_info[MAX_THREADS];
int thread_count = 0;

void dump_mutex(char *name, pthread_mutex_t *mutex)
{
	int i;
	if (mutex->__data.__owner) {
		for (i = 0; i < thread_count; i++) {
			if (thread_info[i].id == mutex->__data.__owner) {
				printf("%s: locked by %s, count=%d\n", name, thread_info[i].name, mutex->__data.__count);
				return;
			}
		}
		printf("%s: owner unknown. count=%d, owner=%d\n", name, mutex->__data.__count, mutex->__data.__owner);
	} else
		printf("%s: unlocked\n", name);
}

extern pthread_mutex_t my_mutex, my_mutex2;

void dump_mutexes(int signum)
{
	int i;
	printf("SIGNAL, DUMPING MUTEXES\n");
	/* add a line here for each mutex
	   EXAMPLE: dump_mutex("my mutex", &g_my_mutex); */
	dump_mutex("my_mutex", &my_mutex);
	dump_mutex("my_mutex2", &my_mutex2);
	if (signum == SIGINT)
		abort();
	else
		signal(signum, dump_mutexes);
}

void register_thread(char *name)
{
	int id = syscall(SYS_gettid);
	printf("registering thread %s: %d\n", name, id);
	thread_info[thread_count].id = id;
	thread_info[thread_count].name = name;
	if (thread_count >= MAX_THREADS) {
		printf("max_threads reached\n");
		return;
	}
	thread_count++;
}

void mutex_debug_init(void)
{
	signal(SIGINT, dump_mutexes);
	signal(SIGUSR1, dump_mutexes);
}
