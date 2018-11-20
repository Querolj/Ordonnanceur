#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <stdatomic.h>


#ifndef SCHED2_H_   
#define SCHED2_H_
struct quicksort_args
{
    int *a;
    int lo, hi;
};
struct pile{
	struct element *head;
}typedef pile;

struct scheduler{
	struct pile *pile;
	struct benchmarks *benchs;
	pthread_t  * t_thread;
	pthread_mutex_t *mutex_signal;
	pthread_mutex_t *t_mutex2; 
	pthread_cond_t *t_cond; //Mutex pour cond
	pthread_mutex_t pile_mutex; //Maintenant un unique mutex pour chaque thread
    int thread_number; //Ajout
    int * working;//Ajout, 1 si le thread travaille 
	//sem_t ** sem_spawn; //semaphore pour sched_spawn
}typedef scheduler;

typedef void (*taskfunc)(void*, struct scheduler *);
struct data{
	taskfunc task; //void* remplacé par taskfunc
	void * closure;
}typedef data;
struct element{
	struct element * prev;
	struct element * next;
	struct data * data;

}typedef element;

struct benchmarks{
	int *task_number;
	int *successful_ws;
	int *unsuccessful_ws;
}typedef benchmarks;

//Ajout
struct arg_thread_fun{
    struct scheduler *s;
    int thread_pos;
}typedef arg_thread_fun;

struct bench_info{
    int *complet_task; //Tableau du nombre de tâche accomplie pour chaque thread
    int lenght; //Taille du tableau
    double time; //Durée de completion du quicksort
}typedef bench_info;

static inline int
sched_default_threads()
{
    return sysconf(_SC_NPROCESSORS_ONLN);
}


void* thread_fun(void* closure);//Ajout

int sched_init(int nthreads, int qlen, taskfunc f, void *closure);
int sched_spawn(taskfunc f, void *closure, struct scheduler *s);
void* pop(struct pile *pile);
void push(struct pile *pile, struct data* );
int is_empty(struct pile *pile);
int Random(int _iMin, int _iMax);
int are_active(struct scheduler *s);
int sleep_thread(struct scheduler * s, int thread_pos);
void set_active(scheduler * s, int thread_pos);
void set_inactive(scheduler * s, int thread_pos);

#endif