// PingPongOS - PingPong Operating System
// Gustavo Gabriel Ripka GRR20203935
// Edison Luiz Matias Junior GRR20211790
// Gabriel Shigueo Ushiwa Kaguimoto Rodrigues GRR20221261

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "semaphore.h"

#define BUFFER_SIZE 5

// --- Semáforos ---
struct semaphore_t *s_buffer; // Exclusão mútua do buffer
struct semaphore_t *s_item;   // Contador de itens (inicia em 0)
struct semaphore_t *s_vaga;   // Contador de vagas (inicia em 5)

// --- Buffer Circular (FIFO) ---
int buffer[BUFFER_SIZE];
int head = 0; // Índice de onde o consumidor tira
int tail = 0; // Índice de onde o produtor insere

// --- Tarefas ---
struct task_t *p1, *p2, *p3;
struct task_t *c1, *c2;

// Corpo do Produtor
void produtor(void *arg) {
    while (1) {
        task_sleep(1000);             // Produz a cada 1 segundo
        int item = rand() % 100;      // Sorteia um número de 0 a 99

        sem_down(s_vaga);             // Pega uma vaga (ou dorme se estiver cheio)
        
        sem_down(s_buffer);           // Tranca o buffer (seção crítica)
        buffer[tail] = item;          // Insere o item
        tail = (tail + 1) % BUFFER_SIZE; // Avança o índice circular
        printf("%s produziu %d\n", (char *)arg, item);
        sem_up(s_buffer);             // Destranca o buffer
        
        sem_up(s_item);               // Avisa que há +1 item disponível
    }
}

// Corpo do Consumidor
void consumidor(void *arg) {
    while (1) {
        sem_down(s_item);             // Pega um item (ou dorme se estiver vazio)
        
        sem_down(s_buffer);           // Tranca o buffer (seção crítica)
        int item = buffer[head];      // Retira o item
        head = (head + 1) % BUFFER_SIZE; // Avança o índice circular
        printf("                             %s consumiu %d\n", (char *)arg, item);
        sem_up(s_buffer);             // Destranca o buffer
        
        sem_up(s_vaga);               // Avisa que há +1 vaga disponível
        
        task_sleep(1000);             // Processa o item durante 1 segundo
    }
}

// Tarefa Principal
void user_main(void *arg) {
    printf("user: inicio\n");

    // Inicialização dos Semáforos
    s_buffer = sem_create(1);           // 1 = Livre para acesso
    s_item   = sem_create(0);           // 0 = Nenhum item no início
    s_vaga   = sem_create(BUFFER_SIZE); // 5 = Todas as vagas livres no início

    // Cria os produtores
    p1 = task_create("p1", produtor, "p1");
    p2 = task_create("p2", produtor, "p2");
    p3 = task_create("p3", produtor, "p3");

    // Cria os consumidores
    c1 = task_create("c1", consumidor, "c1");
    c2 = task_create("c2", consumidor, "c2");

    // Como as tarefas têm loops infinitos (while(1)), fazemos a user_main 
    // esperar por uma delas para não encerrar o SO.
    task_wait(p1);

    printf("user: fim\n");
    task_exit(0);
}