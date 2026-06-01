// PingPongOS - PingPong Operating System
// Gustavo Gabriel Ripka GRR20203935
// Edison Luiz Matias Junior GRR20211790
// Gabriel Shigueo Ushiwa Kaguimoto Rodrigues GRR20221261

#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <signal.h>
#include "ppos.h"
#include "semaphore.h"
#include "task.h"
#include "time.h"
#include "lib/queue.h"

void task_suspend(struct queue_t *queue);
void task_awake(struct task_t *task);

// ESCUDO DE INTERRUPÇÕES
void enter_cs(sigset_t *mask) {
    sigemptyset(mask);
    sigaddset(mask, SIGALRM);
    sigprocmask(SIG_BLOCK, mask, NULL);
}

void leave_cs(sigset_t *mask) {
    sigprocmask(SIG_UNBLOCK, mask, NULL);
}

// SPINLOCKS
void spin_lock(int *lock) {
    while (__sync_lock_test_and_set(lock, 1));
}

void spin_unlock(int *lock) {
    __sync_lock_release(lock);
}

// SEMÁFOROS
struct semaphore_t {
    int counter;
    struct queue_t *queue;
    int lock;
    int destroyed;
};

void sem_init() { }

struct semaphore_t *sem_create(int value) {
    struct semaphore_t *s = malloc(sizeof(struct semaphore_t));
    if (!s) return NULL;

    s->counter = value;
    s->lock = 0;
    s->destroyed = 0;
    s->queue = queue_create();
    
    if (!s->queue) {
        free(s);
        return NULL;
    }
    return s;
}

int sem_down(struct semaphore_t *s) {
    if (s == NULL || s->destroyed) return -1;

    sigset_t mask;
    enter_cs(&mask);

    spin_lock(&s->lock);
    if (s->destroyed) {
        spin_unlock(&s->lock);
        leave_cs(&mask);
        return -1;
    }

    s->counter--;

    if (s->counter < 0) {
        // Libertamos a fechadura antes de dormir (a preempção ainda está desligada, é seguro!)
        spin_unlock(&s->lock);
        
        // A função interna resolve quem é a tarefa atual e põe-na na fila
        task_suspend(s->queue);
        
        // O ucontext restaura a máscara de sinais, voltamos blindados aqui.
        if (s->destroyed) {
            leave_cs(&mask);
            return -1; 
        }
    } else {
        spin_unlock(&s->lock); 
    }

    leave_cs(&mask);
    return 0;
}

int sem_up(struct semaphore_t *s) {
    if (s == NULL || s->destroyed) return -1;

    sigset_t mask;
    enter_cs(&mask);

    spin_lock(&s->lock);
    if (s->destroyed) {
        spin_unlock(&s->lock);
        leave_cs(&mask);
        return -1;
    }

    s->counter++;

    if (s->counter <= 0) {
        struct task_t *t = (struct task_t *) queue_head(s->queue);
        if (t != NULL) {
            queue_del(s->queue, t);
            task_awake(t); 
        }
    }

    spin_unlock(&s->lock);
    leave_cs(&mask);
    return 0;
}

int sem_destroy(struct semaphore_t *s) {
    if (s == NULL || s->destroyed) return -1;

    sigset_t mask;
    enter_cs(&mask);

    spin_lock(&s->lock);
    s->destroyed = 1; 

    while (queue_size(s->queue) > 0) {
        struct task_t *t = (struct task_t *) queue_head(s->queue);
        queue_del(s->queue, t);
        task_awake(t);
    }

    spin_unlock(&s->lock);
    leave_cs(&mask);
    
    
    return 0;
}