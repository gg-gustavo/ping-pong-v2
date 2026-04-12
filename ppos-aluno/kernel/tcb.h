// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 2.0 -- Junho de 2025

// Este arquivo PODE/DEVE ser alterado.

// Gustavo Gabriel Ripka GRR20203935
// Edison Luiz Matias Junior GRR20211790
// Gabriel Shigueo Ushiwa Kaguimoto Rodrigues GRR20221261

// Descritor de tarefas (TCB - Task Control Block).

#ifndef __PPOS_TCB__
#define __PPOS_TCB__

#include "ctx.h"

#define STATUS_READY 0
#define STATUS_RUNNING 1
#define STATUS_SUSPENDED 2
#define STATUS_TERMINATED 3

// Task Control Block (TCB), infos sobre uma tarefa
struct task_t {
    int id;
    char *name;
    struct ctx_t context;
    struct task_t *creator; // Tarefa que a criou (do P1)
    
    int status;             // Estado atual da tarefa
    int vg_id;              // ID da pilha no Valgrind
    
    // Ponteiros para a fila genérica (queue_t)
    struct task_t *prev;
    struct task_t *next;

    int prio_static;
    int prio_dynamic;
};

#endif
