// PingPongOS - PingPong Operating System
// Gustavo Gabriel Ripka GRR20203935
// Edison Luiz Matias Junior GRR20211790
// Gabriel Shigueo Ushiwa Kaguimoto Rodrigues GRR20221261

#include <stdio.h>
#include <stdlib.h>
#include <valgrind/valgrind.h>
#include "task.h"
#include "lib/queue.h"

#define STACK_SIZE (32 * 1024) // 32 KB

struct task_t *current_task = NULL;
struct task_t *previous_task = NULL;

unsigned int task_num = 0;

void task_init()
{
    struct task_t *task_kernel;

    task_kernel = (struct task_t *)malloc(sizeof(struct task_t));

    if (!task_kernel)
        return;

    task_kernel->id = 0;
    task_kernel->name = "kernel";
    task_kernel->context = (struct ctx_t){0};
    task_kernel->creator = NULL;
    task_kernel->status = STATUS_READY;
    task_kernel->prio_static = 0;
    task_kernel->prio_dynamic = 0;

    #ifdef DEBUG
    printf("DEBUG: subsystem task initiated\n");
    #endif

    current_task = task_kernel;
}

struct task_t *task_create(char *name, void (*entry)(void *), void *arg)
{
    struct task_t *task;
    struct ctx_t ctx;
    void *stack;

    if (!(task = (struct task_t *)malloc(sizeof(struct task_t))))
        return NULL;

    if (!(stack = malloc(STACK_SIZE)))
    {
        free(task);
        return NULL;
    }

    if (ctx_create(&ctx, entry, arg, stack, STACK_SIZE) == ERROR)
    {
        free(stack);
        free(task);
        return NULL;
    }

    task->vg_id = VALGRIND_STACK_REGISTER(stack, stack + STACK_SIZE);

    task->id = ++task_num;
    task->name = name;
    task->context = ctx;
    task->creator = current_task;
    task->status = STATUS_READY;
    task->prio_static = 0;
    task->prio_dynamic = 0;

    #ifdef DEBUG
    printf("DEBUG: task %d (%s) create task %d (%s)\n", current_task->id, current_task->name, task->id, task->name);
    #endif

    extern struct task_t *dispatcher_task; 
    extern struct queue_t *ready_queue;    
    extern int user_tasks;                 

    if (task != dispatcher_task) {
        queue_add(ready_queue, task);
        user_tasks++;
    }

    return task;
}

int task_destroy(struct task_t *task)
{
    #ifdef DEBUG
    printf("DEBUG: task %d (%s) destroy task %d (%s)\n", current_task->id, current_task->name, task->id, task->name);
    #endif
    // DESFAZ O REGISTRO DO VALGRIND ANTES DO FREE
    VALGRIND_STACK_DEREGISTER(task->vg_id);

    free(task->context.stack);
    free(task);

    return NOERROR;
}

int task_switch(struct task_t *task)
{
    previous_task = current_task;
    current_task = task ? task : current_task->creator;

    #ifdef DEBUG
    printf("DEBUG: task %d (%s) switch to task %d (%s)\n", previous_task->id, previous_task->name, current_task->id, current_task->name);
    #endif

    if (ctx_swap(&previous_task->context, &current_task->context) == ERROR)
        return ERROR;

    return NOERROR;
}

int task_id(struct task_t *task)
{
    return (task) ? task->id : current_task->id;
}

char *task_name(struct task_t *task)
{
    return (task) ? task->name : current_task->name;
}