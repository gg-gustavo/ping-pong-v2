// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 2.0 -- Junho de 2025

// Este arquivo PODE/DEVE ser alterado.

// Descritor de tarefas (TCB - Task Control Block).

#ifndef __PPOS_TCB__
#define __PPOS_TCB__

#include "ctx.h"

#define STATUS_READY 0
#define STATUS_RUNNING 1
#define STATUS_SUSPENDED 2
#define STATUS_TERMINATED 3

// Task Control Block (TCB), infos sobre uma tarefa
struct task_t
{
    int id;                     // identificador da tarefa
    char *name;                 // nome da tarefa
    struct ctx_t context;       // contexto armazenado da tarefa
    int status;                 // status da tarefa (ready, running, suspended, terminated)
    struct task_t *parent;      // tarefa que criou esta tarefa (NULL para a tarefa inicial)
};

#endif
