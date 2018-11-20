#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <stdatomic.h>
#include <pthread.h>
#include "sched_ws.h"


__thread int current_pos;
struct arg_thread_fun *
new_args_thread_fun(int pos, struct scheduler *s)
{
    struct arg_thread_fun *args = malloc(sizeof(struct arg_thread_fun));
    if (args == NULL)
        return NULL;

    args->thread_pos = pos;
    args->s = s;
    return args;
}

pile *piles_alloc() {
	pile *p = malloc(sizeof(*p));
	if (p != NULL) {
		p->head = NULL;
        p->queue = NULL;
    }
	return p;
}
int is_empty_with_number(struct pile *pile, int i)
{
    if(pile->head == NULL) {
        return 1;
    }
    return 0;
}

int sched_init(int nthreads, int qlen, taskfunc f, void *closure)
{
    printf("Votre machine à %d coeurs.\n", nthreads);
    struct scheduler *sched = malloc(sizeof(struct scheduler));
    pthread_t tab_threads[nthreads];
    sched->t_thread = tab_threads;
    sched->thread_number = nthreads;
    //Initialisation piles
    struct timespec time;
    time.tv_nsec = 100;
    sched->abstime = &time;
    struct pile *p[nthreads];
    // malloc(sizeof(struct pile) * nthreads);
    for (int i = 0; i < nthreads; i++)
    {
        
        p[i]  = malloc(sizeof(struct element));
    }
    sched->piles = p;
    pthread_mutex_init(&sched->t_mutex2, NULL);

    sched->piles_mutex = malloc(sizeof(pthread_mutex_t) * nthreads);
    for (int i = 0; i < nthreads; i++)
    {
        pthread_mutex_init(&sched->piles_mutex[i], NULL);
    }

    pthread_mutex_init(&sched->mutex_signal, NULL);
    if (pthread_cond_init(&sched->t_cond, NULL) < 0)
        perror("sem_init in sched_init");

    sched->working = malloc(sizeof(int) * nthreads);
    for (int i = 0; i < nthreads; i++)
    {
        sched->working[i] = 1;
    }
    for (int i = 0; i < nthreads; i++)
    {
        pthread_create(&sched->t_thread[i], NULL, thread_fun, new_args_thread_fun(i, sched));
    }
    if (closure != NULL)
        sched_spawn(f, closure, sched);
    else
    {
        perror("closure null dans sched_init");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < nthreads; i++)
    {
        pthread_join(sched->t_thread[i], NULL);
    }

    return 0;
}

int sched_spawn(taskfunc f, void *closure, struct scheduler *sched)
{
    struct data *data = malloc(sizeof(struct data));
    if (closure == NULL)
    {
        perror("closure null inside sched_spawn");
        exit(EXIT_FAILURE);
    }

    data->closure = closure;
    data->task = f;
    //printf("spawn %d\n",current_pos);
    int r = Random(0, sched->thread_number-1);
    pthread_mutex_lock(&sched->piles_mutex[r]);

    push(sched->piles[r], data);
    pthread_mutex_unlock(&sched->piles_mutex[r]);

    if (pthread_cond_signal(&sched->t_cond) < 0)
        perror("cond spawn");
    return 0;
}
void mutex(struct scheduler *s)
{
    if ((pthread_mutex_lock(&s->t_mutex2)) < 0)
    {
        perror("pthread_mutex_lock in thread_fun");
        exit(EXIT_FAILURE);
    }
}
void mutex_unlock(struct scheduler *s)
{
    pthread_mutex_unlock(&s->t_mutex2);
}
void broadcast(struct scheduler *s)
{
    if (pthread_cond_broadcast(&s->t_cond) < 0)
    {
        perror("pthread_cond_broadcast in sched_spawn");
        exit(EXIT_FAILURE);
    }
}
void *thread_fun(void *closure)
{
    struct arg_thread_fun *args = (struct arg_thread_fun *)closure;
    int thread_pos = args->thread_pos;
    struct scheduler *s = args->s;
    current_pos = thread_pos;
    
    while (1) //are_active(s) & !is_empty(&s->pile)
    {
        set_active(s, thread_pos);
        
        pthread_mutex_lock(&s->piles_mutex[current_pos]);
        struct data *data = pop(s->piles[current_pos]);
        pthread_mutex_unlock(&s->piles_mutex[current_pos]);
        if (data != NULL)
        {
            //printf("thread %d\n",current_pos);
            data->task((void *)data->closure, s);
        }
        else //si tache, dort paspile vide, on vérifie dans sleep_thread si les autres threads sont actifs
        {
            if( s->piles[current_pos]->head == NULL) {
                workstealing_j(s,thread_pos);
                s->working[current_pos] = 0;
            }
            
           pthread_mutex_lock(&s->piles_mutex[0]);

            if (!are_active (s, thread_pos))
            {                
                printf("exit %d\n",current_pos);
                pthread_mutex_unlock(&s->piles_mutex[0]);
                pthread_exit(NULL);
            }
            pthread_mutex_unlock(&s->piles_mutex[0]);
            sleep(0.1);
        }
    }
    printf("end of thread num %d\n", thread_pos);
    pthread_exit(NULL);
}

int workstealing(struct scheduler *s, int thread_pos)
{
    
    int r = Random(0, s->thread_number);
    int r2 = r;
    while (is_empty_with_number(s->piles[current_pos], r) == 0)
    {
        //write(0,"aah\n",4);
        r++;
        if (r2 % s->thread_number == r)
        {
            pthread_mutex_lock(&s->piles_mutex[thread_pos]);
            struct data *data = defile(s->piles[thread_pos]);
            pthread_mutex_unlock(&s->piles_mutex[thread_pos]);
            if (data != NULL)
            {
                printf("heey");
                data->task((void *)data->closure, s);
            }
            return 1;
        }
    }
    //write(0,"out\n",4);
    return 0;
}

int workstealing_j(struct scheduler *s, int thread_pos)
{
    int r = Random(0, s->thread_number);
    int j;
    for (int i = r; i < r + s->thread_number; i++)
    {
        j = i % s->thread_number;
        if (j != thread_pos) //On cherche à steal
        {
            pthread_mutex_lock(&s->piles_mutex[j]);
            struct data *data = defile(s->piles[j]);
            pthread_mutex_unlock(&s->piles_mutex[j]);
            if (data != NULL)
            {
                data->task((void *)data->closure, s);
            }
            return 1;
        }
    }
    return 0;
}
int are_active(struct scheduler *sched, int thread_pos)
{
    for (int i = 0; i < sched->thread_number; i++)
    {
        if(i ==current_pos);
        else if (sched->working[i] == 1 || sched->piles[i]->head !=NULL)
        {
            return 1;
        }
        
    }
    return 0;
}
void set_active(scheduler *s, int thread_pos)
{
    pthread_mutex_lock(&s->t_mutex2);
    s->working[current_pos] = 1;
    pthread_mutex_unlock(&s->t_mutex2);
}
void set_inactive(scheduler *s, int thread_pos)
{
    pthread_mutex_lock(&s->t_mutex2);
    s->working[current_pos] = 0;
    pthread_mutex_unlock(&s->t_mutex2);
}


int is_empty(struct pile *pile)
{
    if (pile == NULL)
        return 1;
    return 0;
}


void push(pile *pile, data *data) { 
	element *e = malloc(sizeof(*e));
	assert(e != NULL);
	e->data = data;
	e->prev = pile->queue;
	e->next = NULL;
	if (pile->head == NULL) {
		pile->head = e;
        pile->queue = e;
	} else {
		pile->queue->next = e;
		pile->queue = e;
	}
}

void *defile(pile *pile) {
    if (pile == NULL)
    {
        exit(EXIT_FAILURE);
    }
	element *e = pile->head;

    if(pile != NULL && pile->head != NULL) {
        data *dataPop = malloc(sizeof(dataPop));
        dataPop = pile->head->data;
    	if (pile->head == pile->queue) {
    		pile->head = NULL;
            pile->queue = NULL;
        }
    	else {
    		pile->head = e->next;
        }
    	//free(e);
    	return dataPop;
    }
    return NULL;
}

void *pop(pile *pile) {
    //printf("pop");
    if (pile == NULL)
    {
        exit(EXIT_FAILURE);
    }
	element *e = pile->queue;

    if(pile != NULL && pile->queue != NULL) {
        data *dataPop = malloc(sizeof(dataPop));
    	dataPop = e->data;
    	if (pile->head == pile->queue) {
    		pile->head = NULL;
            pile->queue = NULL;
        }
    	else {
    		pile->queue = e->prev;
        }
    	//free(e);
    	return dataPop;
    }
    return NULL;
}
int Random(int _iMin, int _iMax)
{
    return (_iMin + (rand() % (_iMax - _iMin + 1)));
}


/*
void test_push(scheduler * s){
    printf("test de push");
    data * d = malloc(sizeof(data));
    push(*s->piles, d);
}
void test_defile(scheduler * s){
    printf("defile");
    data * d = malloc(sizeof(data));
    defile(*s->piles);
}
void test_pop(scheduler * s){
    printf("pop");
    data * d = malloc(sizeof(data));
    pop(*s->piles);

}*/
void test2(){
    /*struct scheduler * s = malloc(sizeof(struct scheduler));
    struct pile tab [4];
    *s->piles = tab;
    test_push(s);
    test_push(s); 
    test_pop(s);
    test_pop(s);

    test_push(s);
    test_push(s);
    test_defile(s);
    test_pop(s);*/
}