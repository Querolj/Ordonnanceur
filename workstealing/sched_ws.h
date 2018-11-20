#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <stdatomic.h>

#ifndef SCHED_WS_H_   
#define SCHED_WS_H_
struct quicksort_args
{
    int *a;
    int lo, hi;
};
struct pile{
	struct element *head;
	struct element *queue;
}typedef pile;

struct scheduler{
	struct pile **piles;
	pthread_t  * t_thread;
	pthread_mutex_t mutex_signal;
	struct timespec *abstime;
	pthread_mutex_t t_mutex2; 
	pthread_cond_t t_cond; //Mutex pour cond
	pthread_mutex_t *piles_mutex; //Maintenant un unique mutex pour chaque thread
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
void *pop(struct pile *pile);
void push(struct pile *pile, struct data* );
int is_empty(struct pile *pile);
int Random(int _iMin, int _iMax);
int are_active(struct scheduler *s, int thread_pos);
void mutex(struct scheduler *s);
void mutex2(struct scheduler *s);
void mutex_unlock2(struct scheduler *s);
void mutex_unlock(struct scheduler *s);
int sleep_thread(struct scheduler * s, int thread_pos);
void dec_working(scheduler * s);
void inc_working(scheduler * s);
void set_active(scheduler * s, int thread_pos);
void set_inactive(scheduler * s, int thread_pos);
void* defile(struct pile *pile);
int is_empty_with_number(struct pile *piles, int i);
int workstealing(struct scheduler *s, int thread_pos );
int workstealing_j(struct scheduler *, int thread_pos);
void test2();

/*
    WS note :
    1 mutex associé à chaque deques, ils protègent toutes les opérations sur leur deque
    On steal quand ? : Après avoir tenté de pop. + L'indication de travaille engloble plus de truc
*/

#endif