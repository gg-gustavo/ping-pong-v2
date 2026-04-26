// PingPongOS - PingPong Operating System
// Gustavo Gabriel Ripka GRR20203935
// Edison Luiz Matias Junior GRR20211790
// Gabriel Shigueo Ushiwa Kaguimoto Rodrigues GRR20221261

#include "scheduler.h"
#include "lib/queue.h"
#include <stdio.h>

extern struct task_t *current_task;

void sched_init() {
}

void sched_setprio(struct task_t *task, int prio) {
    if (prio < -20) prio = -20;
    if (prio > 20) prio = 20;

    struct task_t *t = (task != NULL) ? task : current_task;
    
    t->prio_static = prio;
    t->prio_dynamic = prio;
}

int sched_getprio(struct task_t *task) {
    struct task_t *t = (task != NULL) ? task : current_task;
    return t->prio_static;
}

struct task_t *scheduler(struct queue_t *ready_queue) {
    if (ready_queue == NULL || queue_size(ready_queue) == 0) {
        return NULL;
    }

    struct task_t *chosen = NULL;
    struct task_t *tp = NULL;
    int min_prio = 100; // Valor alto inicial

    // 1. Encontrar a tarefa com maior prioridade usando as funções da sua LIB
    tp = (struct task_t *) queue_head(ready_queue);
    while (tp != NULL) {
        if (tp->prio_dynamic < min_prio) {
            min_prio = tp->prio_dynamic;
            chosen = tp;
        }
        tp = (struct task_t *) queue_next(ready_queue);
    }

    // 2. Aplicar envelhecimento em todas, exceto a escolhida
    tp = (struct task_t *) queue_head(ready_queue);
    while (tp != NULL) {
        if (tp != chosen) {
            tp->prio_dynamic -= 1;
        }
        tp = (struct task_t *) queue_next(ready_queue);
    }

    // 3. Resetar a escolhida
    if (chosen) {
        chosen->prio_dynamic = chosen->prio_static;
    }

    return chosen;
}