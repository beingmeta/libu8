#include <pthread.h>
#include "mutex_debug.h"

pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t my_mutex2 = PTHREAD_MUTEX_INITIALIZER;

void *my_thread(void *arg)
{
	register_thread("my_thread");
	pthread_mutex_lock(&my_mutex);
	while(1) {
		sleep(1);
	}
}

int main(void)
{
	pthread_t thread;
	register_thread("main");
	mutex_debug_init();
	pthread_mutex_lock(&my_mutex2);
	pthread_create(&thread, NULL, my_thread, NULL);
	dump_mutexes(0);
	sleep(60);
}
