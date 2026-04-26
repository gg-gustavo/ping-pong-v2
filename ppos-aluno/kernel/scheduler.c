// PingPongOS - PingPong Operating System

// Este arquivo PODE/DEVE ser alterado.

// Escalonador de tarefas prontas.
#include "../lib/queue.h"

void sched_init()
{
    
}

struct task_t *scheduler(struct queue_t *ready_queue)
{
    struct task_t *task = queue_head(ready_queue);

    if (task)
        queue_del(ready_queue, task);

    return task;
}