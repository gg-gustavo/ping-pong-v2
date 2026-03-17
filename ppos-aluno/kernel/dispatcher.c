// PingPongOS - PingPong Operating System

#include <stdio.h>
#include "dispatcher.h"
#include "task.h"

extern void user_main(void *arg);   // corpo da tarefa principal

void dispatcher_init()
{
    #ifdef DEBUG
    printf("DEBUG: subsystem dispatcher initiated\n");
    #endif
}

void dispatcher()
{
    struct task_t *task_user;

    task_user = task_create("user", user_main, NULL);
   
    task_switch(task_user);

    #ifdef DEBUG
    printf("DEBUG: dispatcher stopping, no more user tasks\n");
    #endif

    task_destroy(task_user);
}
