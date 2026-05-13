// PingPongOS - PingPong Operating System
// Gustavo Gabriel Ripka GRR20203935
// Edison Luiz Matias Junior GRR20211790
// Gabriel Shigueo Ushiwa Kaguimoto Rodrigues GRR20221261

#include <stdio.h>
#include <stdlib.h>
#include <valgrind/valgrind.h>
#include <string.h>
#include "task.h"
#include "time.h"
#include "lib/queue.h"

#define STACK_SIZE (32 * 1024) // 32 KB

struct task_t *current_task = NULL;
struct task_t *previous_task = NULL;

int task_num = 0;

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
    task_kernel->quantum = QUANTUM;
    task_kernel->quantum_remaining = QUANTUM;

    // Inicializando os campos de tempo da tarefa kernel
    int now = systime();
    task_kernel->start_time = now;
    task_kernel->last_start_time = now;
    task_kernel->cpu_time = 0;
    task_kernel->activations = 1; // Já nasce com 1 ativação pois é a que está rodando

    task_kernel->exit_code = 0;
    task_kernel->waiting_queue = NULL;

    #ifdef DEBUG
    printf("\033[90mDEBUG: subsystem task initiated\n\033[0m");
    #endif

    current_task = task_kernel;
}

struct task_t *task_create(char *name, void (*entry)(void *), void *arg)
{
    struct task_t *task;
    struct ctx_t ctx;
    void *stack;
    int now = systime();

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
    
    if (name){
        task->name = malloc(strlen(name) + 1);
        strcpy(task->name, name);
    }
    else
        task->name = NULL;
    //task->name = malloc(strlen(name) + 1);
    //strcpy(task->name, name);
    
    task->context = ctx;
    
    #ifdef DEBUG
    if (current_task)
        printf("\033[90mDEBUG: task %d (%s) create task %d (%s)\n\033[0m",
           current_task->id, current_task->name, task->id, task->name);
    else
        printf("\033[90mDEBUG: create task %d (%s)\n\033[0m", task->id, task->name);
    #endif

    task->creator = current_task;
    task->status = STATUS_READY;
    task->prio_static = 0;
    task->prio_dynamic = 0;
    task->quantum = QUANTUM;
    task->quantum_remaining = QUANTUM;

    task->start_time = now;
    task->last_start_time = now;
    task->cpu_time = 0;
    task->activations = 0;

    task->wake_time = 0;

    task->exit_code = 0;

    if ((task->waiting_queue = queue_create()) == NULL)
        return NULL;

    #ifdef DEBUG
    printf("\033[90mDEBUG: task %d (%s) create task %d (%s)\n\033[0m", current_task->id, current_task->name, task->id, task->name);
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
    printf("\033[90mDEBUG: task %d (%s) destroy task %d (%s)\n\033[0m", current_task->id, current_task->name, task->id, task->name);
    #endif
    // DESFAZ O REGISTRO DO VALGRIND ANTES DO FREE
    VALGRIND_STACK_DEREGISTER(task->vg_id);

    queue_destroy(task->waiting_queue);
    free(task->name);
    free(task->context.stack);
    free(task);

    return NOERROR;
}

int task_switch(struct task_t *task)
{
    int now = systime();

    previous_task = current_task;

    current_task = task ? task : current_task->creator;


    //contabiliza tempo de CPU da tarefa atual
    if (previous_task)
    {
        previous_task->cpu_time += now - previous_task->last_start_time;
    }

    current_task->activations++;

    current_task->last_start_time = now;

    #ifdef DEBUG
    // printf("\033[90mDEBUG: task %d (%s) switch to task %d (%s)\n\033[0m", previous_task->id, previous_task->name, current_task->id, current_task->name);
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