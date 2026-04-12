// PingPongOS - PingPong Operating System

// Este arquivo PODE/DEVE ser alterado.
// Gustavo Gabriel Ripka GRR20203935
// Edison Luiz Matias Junior GRR20211790
// Gabriel Shigueo Ushiwa Kaguimoto Rodrigues GRR20221261

// Escalonador de tarefas prontas.

#include "scheduler.h"
#include "lib/queue.h"

void sched_init() {
    // Para o FCFS, não precisamos inicializar nada de especial
}

struct task_t *scheduler(struct queue_t *ready_queue) {
    // CORREÇÃO: Usando a sua função queue_size() ao invés de acessar a struct
    if (ready_queue == NULL || queue_size(ready_queue) == 0) {
        return NULL;
    }
    
    // CORREÇÃO: Usando a sua função queue_head() para pegar o primeiro da fila
    return (struct task_t *) queue_head(ready_queue);
}

void sched_setprio(struct task_t *task, int prio) {}
int sched_getprio(struct task_t *task) { return 0; }
