// PingPongOS - PingPong Operating System

// Gustavo Gabriel Ripka GRR20203935
// Edison Luiz Matias Junior GRR20211790
// Gabriel Shigueo Ushiwa Kaguimoto Rodrigues GRR20221261


#include <stdio.h>
#include "dispatcher.h"
#include "task.h"
#include "scheduler.h"

extern struct task_t *current_task;
extern void user_main(void *arg);   // corpo da tarefa principal

struct queue_t *ready_queue = NULL; //fila de prontas
struct queue_t *suspended_queue = NULL; //fila de suspensas
struct task_t *dispatcher_task = NULL; //Tarefa no dispatcher

int user_tasks = 0;

void dispatcher_init()
{
    ready_queue = queue_create();
    suspended_queue = queue_create();
    #ifdef DEBUG
    printf("DEBUG: subsystem dispatcher initiated\n");
    #endif
}

void dispatcher()
{
    struct task_t *next;
    dispatcher_task = current_task;

    user_tasks++;

     while (user_tasks > 0)
    {
        // pega próxima tarefa
        next = scheduler(ready_queue);

        if (next)
        {
            task_run(next);

            // voltou da execução → checa status
            if (next->status == STATUS_TERMINATED)
            {
                task_destroy(next);
                user_tasks--;
            }
        }
    }

    #ifdef DEBUG
    printf("DEBUG: dispatcher stopping, no more user tasks\n");
    #endif
}

void task_run(struct task_t *task)
{
    if (!task)
        return;

    // remove da ready_queue (se ainda estiver lá)
    if (queue_has(ready_queue, task))
        queue_del(ready_queue, task);

    // muda status
    task->status = STATUS_RUNNING;

    // troca de contexto
    task_switch(task);
}

void task_suspend(struct queue_t *queue)
{
    if (!current_task)
        return;

    // muda status
    current_task->status = STATUS_SUSPENDED;

    // remove da ready_queue (se estiver lá)
    if (queue_has(ready_queue, current_task))
        queue_del(ready_queue, current_task);

    // adiciona na fila de suspensão
    if (queue)
        queue_add(queue, current_task);
    else
        queue_add(suspended_queue, current_task);

    // troca pro dispatcher
    task_switch(dispatcher_task);
}

void task_awake(struct task_t *task)
{
    if (!task)
        return;

    // remove da fila de suspensos
    queue_del(suspended_queue, task);

    // muda status
    task->status = STATUS_READY;

    // volta pra ready_queue
    queue_add(ready_queue, task);
}