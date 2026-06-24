// PingPongOS - PingPong Operating System

// Este arquivo PODE/DEVE ser alterado.

// Gerência de um dispositivo orientado a blocos.

// INCLUDES E HEADERS
// PingPongOS - PingPong Operating System
// Gerência de um dispositivo orientado a blocos.

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "hardware/cpu.h"
#include "hardware/disk.h"
#include "block.h"
#include "tcb.h"
#include "semaphore.h"

#define DISK_OP_READ  1
#define DISK_OP_WRITE 2

extern struct task_t *current_task;

struct disk_request_t {
    int op_type;
    int block;
    void *buffer;
    struct task_t *task;
    struct semaphore_t *sync_sem;
    struct disk_request_t *next;
};

static struct task_t *disk_manager_task = NULL;
static struct disk_request_t *request_queue_head = NULL;
static struct disk_request_t *request_queue_tail = NULL;
static struct disk_request_t *current_request = NULL;

// Mutex da fila e semáforo para acordar o gerente
static struct semaphore_t *queue_mutex;
static struct semaphore_t *request_sem;

volatile int ocorreu_irq = 0;

// Trata a interrupção (IRQ) do disco
void trata_irq_disco(int signum) {
    ocorreu_irq = 1;
    sem_up(request_sem); // Acorda o gerente
}

// Funções auxiliares de manipulação da fila
static void enqueue(struct disk_request_t *req) {
    req->next = NULL;
    if (!request_queue_tail) {
        request_queue_head = req;
        request_queue_tail = req;
    } else {
        request_queue_tail->next = req;
        request_queue_tail = req;
    }
}

static struct disk_request_t *dequeue() {
    if (!request_queue_head) {
        return NULL;
    }

    struct disk_request_t *r = request_queue_head;
    request_queue_head = request_queue_head->next;

    if (!request_queue_head) {
        request_queue_tail = NULL;
    }

    r->next = NULL;
    return r;
}

// Envia um pedido de E/S para o disco
int envia_pedido_disco(int op, int block, void *buffer) {
    struct disk_request_t *req = malloc(sizeof(*req));
    if (!req) {
        return -1;
    }

    req->op_type = op;
    req->block = block;
    req->buffer = buffer;
    req->task = current_task;
    req->sync_sem = sem_create(0);

    // Insere na fila de forma protegida
    sem_down(queue_mutex);
    enqueue(req);
    sem_up(queue_mutex);

    sem_up(request_sem);    // Avisa o gerente de disco
    sem_down(req->sync_sem); // Dorme esperando a operação terminar

    sem_destroy(req->sync_sem);
    free(req);

    return 0;
}

// Corpo do Gerente de Disco
void disk_manager_body(void *arg) {
    while (1) {
        sem_down(request_sem); // Espera pedido ou IRQ
        sem_down(queue_mutex);

        // 1. Finalizou operação anterior?
        if (ocorreu_irq) {
            ocorreu_irq = 0;
            if (current_request) {
                sem_up(current_request->sync_sem);
                current_request = NULL;
            }
        }

        // 2. Dispara a próxima operação se o hardware estiver livre
        if (!current_request && request_queue_head && hw_disk_cmd(DISK_CMD_STATUS, 0, 0) == DISK_STATUS_IDLE) {
            current_request = dequeue();
            int cmd = (current_request->op_type == DISK_OP_READ) ? DISK_CMD_READ : DISK_CMD_WRITE;
            hw_disk_cmd(cmd, current_request->block, current_request->buffer);
        }

        // 3. Condição segura de encerramento
        if (disk_manager_task && disk_manager_task->status == STATUS_TERMINATED && !request_queue_head && !current_request) {
            sem_up(queue_mutex);
            break;
        }

        sem_up(queue_mutex);
        task_yield();
    }

    task_exit(0);
}

// Inicializa o subsistema de disco
void block_init(char *disk_image) {
    extern int user_tasks;

    if (hw_disk_cmd(DISK_CMD_INIT, 0, disk_image) < 0) {
        perror("Erro ao inicializar o disco");
        exit(1);
    }

    queue_mutex = sem_create(1);
    request_sem = sem_create(0);

    if (hw_irq_handle(IRQ_DISK, trata_irq_disco) < 0) {
        perror("Erro ao registrar IRQ do disco");
        exit(1);
    }

    disk_manager_task = task_create("Disk Manager", disk_manager_body, NULL);
    user_tasks--; // Ajusta contador de tarefas de usuário do PPOS
}

// Finaliza o subsistema de disco de forma síncrona
void block_stop(char *disk_image) {
    if (!disk_manager_task) {
        return;
    }

    disk_manager_task->status = STATUS_TERMINATED;
    sem_up(request_sem); // Garante que o gerente acorde para avaliar o fim

    // Espera todas as tarefas na fila terminarem
    while (request_queue_head || current_request) {
        task_yield();
    }

    task_yield();

    sem_destroy(queue_mutex);
    sem_destroy(request_sem);
    disk_manager_task = NULL;
}

// Retorna o tamanho do bloco do disco
int block_size() {
    return hw_disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
}

// Retorna a quantidade de blocos total
int block_blocks() {
    return hw_disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
}

// Interface pública de leitura
int block_read(int block, void *buffer) {
    return envia_pedido_disco(DISK_OP_READ, block, buffer);
}

// Interface pública de escrita
int block_write(int block, void *buffer) {
    return envia_pedido_disco(DISK_OP_WRITE, block, buffer);
}