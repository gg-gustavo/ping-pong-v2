// PingPongOS - PingPong Operating System
// Gustavo Gabriel Ripka GRR20203935
// Edison Luiz Matias Junior GRR20211790
// Gabriel Shigueo Ushiwa Kaguimoto Rodrigues GRR20221261

#include <string.h>
#include "ppos.h"
#include "mqueue.h"
#include "semaphore.h"

// ESTRUTURA DA FILA DE MENSAGENS
struct mqueue_t {
    void *buffer; // Vetor contíguo para o buffer circular
    int max_msgs; // Capacidade máxima de mensagens
    int msg_size; // Tamanho de cada mensagem em bytes
    
    int head; // Índice de leitura (Consumidor)
    int tail; // Índice de escrita (Produtor)
    int count; // Quantidade atual de mensagens
    int destroyed; // Flag de segurança (1 = destruída)
    
    struct semaphore_t *s_vaga; // Contador de vagas livres
    struct semaphore_t *s_item; // Contador de itens disponíveis
    struct semaphore_t *s_buffer; // MutEx para proteger o acesso à fila
};

void mqueue_init() {
    // Vazio, a inicialização real de cada fila ocorre no mqueue_create
}

// CRIAÇÃO DA FILA
struct mqueue_t *mqueue_create(int max_msgs, int msg_size) {
    if (max_msgs <= 0 || msg_size <= 0) return NULL;

    struct mqueue_t *q = mem_alloc(sizeof(struct mqueue_t));
    if (!q) return NULL;

    // Aloca o buffer circular de uma só vez (sem cast para unsigned/size_t, respeitando a mini-libc)
    q->buffer = mem_alloc(max_msgs * msg_size);
    if (!q->buffer) {
        mem_free(q);
        return NULL;
    }

    q->max_msgs = max_msgs;
    q->msg_size = msg_size;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->destroyed = 0;

    q->s_vaga   = sem_create(max_msgs); // Começa com todas as vagas livres
    q->s_item   = sem_create(0); // Começa sem nenhum item disponível
    q->s_buffer = sem_create(1); // MutEx livre para uso

    // Rollback rigoroso em caso de falha parcial de alocação de semáforos
    if (!q->s_vaga || !q->s_item || !q->s_buffer) {
        if (q->s_vaga)   sem_destroy(q->s_vaga);
        if (q->s_item)   sem_destroy(q->s_item);
        if (q->s_buffer) sem_destroy(q->s_buffer);
        mem_free(q->buffer);
        mem_free(q);
        return NULL; 
    }

    return q;
}

// ENVIO (PRODUTOR)
int mqueue_send(struct mqueue_t *queue, void *msg) {
    if (!queue || queue->destroyed || !msg) return -1;

    // Tenta pegar numa vaga. Se sem_down retornar < 0, a fila foi destruída!
    if (sem_down(queue->s_vaga) < 0) return -1;
    
    // Tranca a secção crítica para mexer no buffer
    if (sem_down(queue->s_buffer) < 0) {
        sem_up(queue->s_vaga); // Rollback da vaga se o MutEx falhar
        return -1;
    }

    // Verificação de segurança adicional
    if (queue->destroyed) {
        sem_up(queue->s_buffer);
        return -1;
    }

    // Copia os bytes da mensagem para a posição "tail" do buffer
    void *dest = (char *)queue->buffer + (queue->tail * queue->msg_size);
    memcpy(dest, msg, queue->msg_size);
    
    // Avança o índice do Produtor de forma circular
    queue->tail = (queue->tail + 1) % queue->max_msgs;
    queue->count++;

    // Liberta a secção crítica e avisa que há um novo item
    sem_up(queue->s_buffer);
    sem_up(queue->s_item);

    return 0;
}

// RECEÇÃO (CONSUMIDOR)
int mqueue_recv(struct mqueue_t *queue, void *msg) {
    if (!queue || queue->destroyed || !msg) return -1;

    // Tenta pegar num item. Se sem_down retornar < 0, a fila foi destruída!
    if (sem_down(queue->s_item) < 0) return -1;
    
    // Tranca a secção crítica
    if (sem_down(queue->s_buffer) < 0) {
        sem_up(queue->s_item); // Rollback do item se o MutEx falhar
        return -1;
    }

    // Verificação de segurança adicional
    if (queue->destroyed) {
        sem_up(queue->s_buffer);
        return -1;
    }

    // Copia os bytes da posição "head" do buffer para o destino do utilizador
    void *src = (char *)queue->buffer + (queue->head * queue->msg_size);
    memcpy(msg, src, queue->msg_size);
    
    // Avança o índice do Consumidor de forma circular
    queue->head = (queue->head + 1) % queue->max_msgs;
    queue->count--;

    // Liberta a secção crítica e avisa que há uma nova vaga
    sem_up(queue->s_buffer);
    sem_up(queue->s_vaga);

    return 0;
}

// DESTRUIÇÃO DA FILA
int mqueue_destroy(struct mqueue_t *queue) {
    if (!queue || queue->destroyed) return -1;

    // Tranca a estrutura temporariamente para mudar a flag
    sem_down(queue->s_buffer);
    queue->destroyed = 1;
    sem_up(queue->s_buffer);

    sem_destroy(queue->s_vaga);
    sem_destroy(queue->s_item);
    sem_destroy(queue->s_buffer);

    if (queue->buffer) {
        mem_free(queue->buffer);
        queue->buffer = NULL;
    }

    return 0;
}

// VERIFICAÇÃO DO TAMANHO
int mqueue_msgs(struct mqueue_t *queue) {
    if (!queue || queue->destroyed) return -1;
    
    sem_down(queue->s_buffer);
    int current_count = queue->count;
    sem_up(queue->s_buffer);
    
    return current_count;
}