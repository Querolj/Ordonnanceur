#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <stdatomic.h>
#include <pthread.h>
#include "sched2.h"
/*Lance les threads
int pthread_create(pthread_t * thread, pthread_attr_t * attr, void *(*start_routine) (void *), void *arg);
*/

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

int sched_init(int nthreads, int qlen, taskfunc f, void *closure)
{
    printf("Votre machine à %d coeurs.\n", nthreads);
    struct scheduler *sched = malloc(sizeof(struct scheduler));
    pthread_t tab_threads[nthreads];
    sched->t_thread = tab_threads;
    sched->thread_number = nthreads;
    
    sched->pile = malloc(sizeof(struct pile));
    pthread_mutex_init(&sched->pile_mutex, NULL);
    
    sched->t_mutex2 = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(sched->t_mutex2, NULL);
    sched->mutex_signal = malloc(sizeof(pthread_mutex_t));
    sched->t_cond = malloc(sizeof(pthread_cond_t));
    pthread_mutex_init(sched->mutex_signal, NULL);
    if (pthread_cond_init(sched->t_cond, NULL) < 0)
        perror("sem_init in sched_init");
    
    sched->benchs = malloc(sizeof(benchmarks));
    sched->benchs->task_number = malloc(sizeof(int)*nthreads);
    sched->working = malloc(sizeof(int)*nthreads);
    for (int i = 0; i < nthreads; i++)
    {
        sched->benchs->task_number[i] = 0;
        sched->working[i] = 0;
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
    ////////////////////// STATISTIQUES
    for (int i = 0; i < nthreads; i++){
        printf("thread num %d, tâches : %d\n",i,sched->benchs->task_number[i]);
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
    pthread_mutex_lock(&sched->pile_mutex);

    push(sched->pile, data);
    pthread_mutex_unlock(&sched->pile_mutex);

    if (pthread_cond_signal(sched->t_cond) < 0)
        perror("cond spawn");
    return 0;
}



void *thread_fun(void *closure)
{
    struct arg_thread_fun *args = (struct arg_thread_fun *)closure;
    int thread_pos = args->thread_pos;
    struct scheduler *s = args->s;

    while (1) //are_active(s) & !is_empty(&s->pile)
    {
        set_active(s, thread_pos);
        pthread_mutex_lock(&s->pile_mutex);
        struct data *data = pop(s->pile);
        pthread_mutex_unlock(&s->pile_mutex);
        if (data != NULL)
        {
            s->benchs->task_number[thread_pos]++;
            data->task((void *)data->closure, s);
        }
        else//si tache, dort paspile vide, on vérifie dans sleep_thread si les autres threads sont actifs
        {
            set_inactive(s,thread_pos);
            if(!are_active(s))
            {
                pthread_cond_signal(s->t_cond);
                pthread_exit(NULL);
            }
            //sleep(0.1);
            pthread_cond_wait(s->t_cond, s->t_mutex2);
        }
    }

    printf("end of thread num %d\n", thread_pos);
    pthread_exit(NULL);
}

int are_active(struct scheduler *sched)
{
    if (pthread_mutex_lock(sched->t_mutex2) < 0)
        perror("mutex2");
    for(int i =0;i<sched->thread_number;i++){
        if(sched->working[i]==1){
            pthread_mutex_unlock(sched->t_mutex2);
            return 1;
        }
            
    }
    pthread_mutex_unlock(sched->t_mutex2);
    return 0;
}
void set_active(scheduler * s, int thread_pos)
{
    pthread_mutex_lock(s->t_mutex2);
    s->working[thread_pos] = 1;
    pthread_mutex_unlock(s->t_mutex2);
}
void set_inactive(scheduler * s, int thread_pos)
{
    pthread_mutex_lock(s->t_mutex2);
    s->working[thread_pos] = 0;
    pthread_mutex_unlock(s->t_mutex2);
}

int is_empty(struct pile *pile)
{
    if (pile == NULL)
        return 1;
    return 0;
}
void *pop(struct pile *pile)
{
    if (pile == NULL)
    {
        perror("pile null inside pop");
        exit(EXIT_FAILURE);
    }
    struct data *data = malloc(sizeof(data));
    data = NULL;
    struct element *tmp = pile->head;
    if (pile != NULL && pile->head != NULL)
    {
        data = tmp->data;
        pile->head = tmp->next;
        free(tmp);
    }

    return data;
}

void push(struct pile *pile, struct data *data)
{
    //printf("push\n");
    struct element *e = malloc(sizeof(struct element));
    if (pile == NULL || e == NULL)
    {
        perror("push");
        exit(EXIT_FAILURE);
    }
    e->data = data;
    e->next = pile->head;
    pile->head = e;
    
}


int Random(int _iMin, int _iMax)
{
    return (_iMin + (rand() % (_iMax - _iMin + 1)));
}