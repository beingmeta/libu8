#include <pthread.h>
/* mutex-debug.c */
void dump_mutex(char *name, pthread_mutex_t *mutex);
void dump_mutexes(int signum);
void register_thread(char *name);
void mutex_debug_init(void);
