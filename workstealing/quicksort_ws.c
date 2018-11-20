#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
//#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <assert.h>
#include <getopt.h>
#include "sched_ws.h"

int partition(int *a, int lo, int hi)
{
    int pivot = a[lo];
    int i = lo - 1;
    int j = hi + 1;
    int t;
    while (1)
    {
        do
        {
            i++;
        } while (a[i] < pivot);

        do
        {
            j--;
        } while (a[j] > pivot);

        if (i >= j)
            return j;

        t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
}

struct quicksort_args *
new_args(int *a, int lo, int hi)
{
    struct quicksort_args *args = malloc(sizeof(struct quicksort_args));
    if (args == NULL)
        return NULL;

    args->a = a;
    args->lo = lo;
    args->hi = hi;
    return args;
}

void quicksort_serial(int *a, int lo, int hi)
{
    int p;

    if (lo >= hi)
        return;

    p = partition(a, lo, hi);
    quicksort_serial(a, lo, p);
    quicksort_serial(a, p + 1, hi);
}

void quicksort(void *closure, struct scheduler *s)
{
    //printf("quicksort running\n");
    struct quicksort_args *args = (struct quicksort_args *)closure;
    int *a = args->a;
    int lo = args->lo;
    int hi = args->hi;
    int p;
    int rc;

    free(closure);

    if (lo >= hi)
        return;

    if (hi - lo <= 128)
    {
        quicksort_serial(a, lo, hi);
        return;
    }

    p = partition(a, lo, hi);
    rc = sched_spawn(quicksort, new_args(a, lo, p), s);
    assert(rc >= 0);
    rc = sched_spawn(quicksort, new_args(a, p + 1, hi), s);
    assert(rc >= 0);
}

int f(void *arg, scheduler *s)
{
    printf("cool\n");
    return 1;
}
void *f2(void *arg, scheduler *s)
{
    printf("cool 2\n");
}
element *pile_new(void)
{
    return (NULL);
}
/*
void test()
{
    
    int thread_number = 4;
    struct scheduler *s = malloc(sizeof(scheduler));
    s->pile = malloc(sizeof(element *));
   
    int *a = malloc(sizeof(int));
    *a = 1;
    int lo = 5;
    int p = 9;

    //sched_spawn((taskfunc) quicksort, (void*)(new_args(a, lo, p)), s);
    struct data *d = malloc(sizeof(data));
    d->task = (taskfunc)f2;
    d->closure = (void *)(new_args(a, lo, p));
    push(&s->pile, d);

    struct data *data2 = pop(&s->pile);
    if (data2 != NULL)
        data2->task((void *)data2->closure, s);
    data2 = pop(&s->pile);
    if (data2 != NULL)
        data2->task((void *)data2->closure, s);

    s->pile = malloc(sizeof(element *));
    struct data *d3 = malloc(sizeof(data));
    d3->task = (taskfunc)f;
    d3->closure = (void *)(new_args(a, lo, p));
    push(&s->pile,d3);

    data *d2 = malloc(sizeof(data));
    d2->task = (taskfunc)f;
    d2->closure = (void *)(new_args(a, lo, p));
    push(&s->pile,d2);
    
    data2 = pop(&s->pile);
    if (data2 != NULL)
        data2->task((void *)data2->closure, s);
    data2 = pop(&s->pile);
    if (data2 != NULL)
        data2->task((void *)data2->closure, s);
    
    
}*/
int main(int argc, char **argv)
{
    //test();
    
    int *a;
    struct timespec begin, end;
    double delay;
    int rc;
    int n = 10 * 1024 * 1024;
    int nthreads = sched_default_threads();
    int serial = 0;
    while (1)
    {
        int opt = getopt(argc, argv, "sn:t:");
        if (opt < 0)
            break;
        switch (opt)
        {
        case 's':
            serial = 1;
            break;
        case 'n':
            n = atoi((int)opt);
            printf("%d",n);
            if (n <= 0)
                goto usage;
            break;
        case 't':
            nthreads = atoi(opt);
            if (nthreads <= 0)
                goto usage;
            break;
        default:
            goto usage;
        }
    }
    //n = 10 * 1024 * 1024;
    n = 10000000                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        ;
    nthreads = 4;
    a = malloc(n * sizeof(int));
    for (int i = 0; i < n; i++)
    {
        if (i % 2 == 0)
            a[i] = i;
        else
            a[i] = -i;
    }

    clock_gettime(CLOCK_MONOTONIC, &begin);
    
    if (serial)
    {
        quicksort_serial(a, 0, n - 1);
    }
    else
    {
        
        rc = sched_init(nthreads, (n + 127) / 128,
                        quicksort, new_args(a, 0, n - 1));
        assert(rc >= 0);
    }

     clock_gettime(CLOCK_MONOTONIC, &end);
    delay = end.tv_sec + end.tv_nsec / 1000000000.0 -
            (begin.tv_sec + begin.tv_nsec / 1000000000.0);
    //delay = delay*1000;
    printf("Done in %lf s.\n", delay);

    for (int i = 0; i < n - 1; i++)
    {
        assert(a[i] <= a[i + 1]);
    }

    free(a);
    return 0;

usage:
    printf("quicksort [-n size] [-t threads] [-s]\n");
    
    return 1;
}
