// PingPongOS - PingPong Operating System

// Gustavo Gabriel Ripka GRR20203935
// Edison Luiz Matias Junior GRR20211790
// Gabriel Shigueo Ushiwa Kaguimoto Rodrigues GRR20221261

#include <stdio.h>
#include "time.h"
#include "dispatcher.h"
#include "scheduler.h"
#include "task.h"
#include "lib/queue.h"

// Variáveis globais do Dispatcher
struct task_t *dispatcher_task; // Ponteiro para a tarefa do dispatcher (Main)
struct queue_t *ready_queue;
int user_tasks = 0; // Contador de tarefas de usuário

extern void user_main(void *arg);

void dispatcher_init() {
    // Inicializa a fila de prontas
    ready_queue = queue_create(); 
    #ifdef DEBUG
    printf("DEBUG: subsystem dispatcher initiated\n");
    #endif
}

void dispatcher() {
    extern struct task_t *current_task; // Vem do task.c
    
    // O dispatcher captura a tarefa atual que está rodando (a Main)
    dispatcher_task = current_task;

    struct task_t *next_task;

    // Cria a tarefa de usuário inicial
    task_create("user", user_main, NULL);

    // O loop principal (O Cérebro)
    while (user_tasks > 0) {
        next_task = scheduler(ready_queue);

        if (next_task != NULL) {
            task_run(next_task);

            // Ao voltar do task_run, verificamos o que aconteceu com a tarefa
            if (next_task->status == STATUS_TERMINATED) {
                printf("PPOS: task %d (%s) exit code %d, %u ms elapsed time, %u ms cpu time, %u activations\n",
                    next_task->id, next_task->name, next_task->exit_code, next_task->end_time - next_task->start_time,
                    next_task->cpu_time, next_task->activations);
            }
        }
    }
}

void task_run(struct task_t *task) {
    // Tira da fila de prontas e muda o status
    queue_del(ready_queue, task);
    task->status = STATUS_RUNNING;
    
    // Reseta o quantum da tarefa
    task->quantum_remaining = task->quantum;
    
    // Passa a CPU para ela
    task_switch(task);
}

void task_yield() {
    extern struct task_t *current_task; // Vem do task.c
    
    current_task->status = STATUS_READY;
    // Coloca a tarefa de volta na fila de prontas
    queue_add(ready_queue, current_task);
    
    // Volta pro dispatcher
    task_switch(dispatcher_task);
}

void task_exit(int exit_code) {
    extern struct task_t *current_task;
    struct task_t *waiting_task;

    current_task->status = STATUS_TERMINATED;
    current_task->exit_code = exit_code;
    user_tasks--; // Uma tarefa a menos no sistema

    current_task->end_time = systime();

    #ifdef DEBUG
    printf("DEBUG: task %d (%s) has finished\n", current_task->id, current_task->name);
    #endif

    // Acorda todas as tarefas que estavam esperando por essa tarefa
    while ((waiting_task = queue_item(current_task->waiting_queue)))
    {
        task_awake(waiting_task);
        queue_del(current_task->waiting_queue, waiting_task);
    }
    
    // Volta pro dispatcher
    task_switch(dispatcher_task);
}

void task_suspend(struct queue_t *queue) {
    extern struct task_t *current_task;
    
    // Remove da fila de prontas
    queue_del(ready_queue, current_task);
    current_task->status = STATUS_SUSPENDED;
    
    // Insere na fila de espera
    if (queue != NULL) {
        queue_add(queue, current_task);
    }
    
    // Volta pro dispatcher
    task_switch(dispatcher_task);
}

void task_awake(struct task_t *task) {
    // Acorda a tarefa: muda o status e joga na ready_queue
    task->status = STATUS_READY;

    #ifdef DEBUG
    printf("DEBUG: task %d (%s) awake\n", task->id, task->name);
    #endif

    queue_add(ready_queue, task);
}

int task_wait(struct task_t *task)
{
    extern struct task_t *current_task;

    if (task == NULL || task->status == STATUS_TERMINATED)
        return ERROR;

    if (task->status == STATUS_SUSPENDED)
        return task->exit_code;

    #ifdef DEBUG
    printf("DEBUG: task %d (%s) suspended, waiting for task %d (%s)\n",
        current_task->id, current_task->name, task->id, task->name);
    #endif

    task_suspend(task->waiting_queue);

    return task->exit_code;
}
