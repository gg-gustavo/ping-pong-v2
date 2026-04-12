// PingPongOS - PingPong Operating System
// Gustavo Gabriel Ripka - GRR20203935 05 de Março de 2026
// Implementação do TAD fila genérica

#include "queue.h"
#include <stdlib.h>
#include <stdio.h>  

struct queue_node_t{
    void *item; //caixa azul 
    struct queue_node_t *next; //caixa verde
}; 

struct queue_t{
    struct queue_node_t *head; //Ponteiro para cabeça
    struct queue_node_t *tail; //Ponteiro para calda
    struct queue_node_t *iterator; //Ponteiro que itera dentro da fila
    int size;  // Quantidade de nós na fila
};

struct queue_t *queue_create(){
    // criamos um ponteiro que sera alocado para o tamanho da struct
    struct queue_t *q = malloc(sizeof(struct queue_t));
    // testamos se vamos conseguir mallocar
    if (q == NULL){ 
        return NULL;
    }
    //apontamos todas as variaveis para NULL e setamos o tamanho em 0
    q->head = NULL;
    q->tail = NULL;
    q->iterator = NULL;
    q->size = 0;

    //devolvemos o ponteiro
    return q;
}

int queue_destroy(struct queue_t *queue){
    //vamos testar se existe endereço de memoria para varrer
    if (queue == NULL){
        return ERROR;
    }
    // Começamos pela cabeça
    struct queue_node_t *current = queue->head;
    struct queue_node_t *prox;
    while (current != NULL) {
        prox = current->next; // Salva o próximo antes de deletar o atual
    //  Se o seu projeto exigir que você limpe o dado (item) também:
    free(current->item); 
        
    free(current);    // Deleta o nó (caixa azul/verde da sua lógica)
    current = prox;
    }
    // caso não seja NULL, passamos a vassoura
    free(queue); 
    return NOERROR;
}

int queue_add(struct queue_t *queue, void *item){
    //testamos se o ponteiro ou o item apontam para NULL
    if (queue == NULL || item == NULL){
    return ERROR;
    }
    //vamos criar o ponteiro do tipo caixa verde 
    struct queue_node_t *new_node = malloc(sizeof(struct queue_node_t));

    //como estamos usando uma função que retorna inteiro, não devolvemos NULL
    if (new_node == NULL){
        return ERROR;
    }

    new_node->item = item;
    new_node->next = NULL;
    //fila vazia, todo mundo aponta para a mesma caixa
    if (queue->size == 0){
        queue->head = new_node;
        queue->iterator = new_node;
        queue->tail = new_node;
    }
    //vamos ter o ultimo vai receber o proximo, logo apos o poximo vira ultimo
    else{
        queue->tail->next = new_node;
        queue->tail = new_node;
    }
    
    //nodo adicionado, aumentou nossa fila
    queue->size++;
    
    return NOERROR;

}

int queue_del(struct queue_t *queue, void *item) {
    //barreira de segurança
    if (queue == NULL || item == NULL || queue->size == 0) {
        return ERROR;
    }

    //criamos dois ptrs viajantes
    struct queue_node_t *current = queue->head;
    struct queue_node_t *prev = NULL; 

    //andamos até achar o item ou a fila acabar
    while (current != NULL && current->item != item) {
        prev = current;          
        current = current->next; 
    }

    //se o current caiu no void, o item não estava na fila
    if (current == NULL) {
        return ERROR;
    }

    //se vamos remover o primeiro da fila
    if (prev == NULL) {
        queue->head = current->next; 
    } 
    //se está no meio ou no fim
    else {
        prev->next = current->next;  
    }
    //se vamos remover o ÚLTIMO da fila
    if (current == queue->tail) {
        queue->tail = prev; 
    }
    //iterador estava apontando pro cara removido, tiramos ele de lá
    if (queue->iterator == current) {
        queue->iterator = current->next; 
    }
    //liberar para não vazar memória
    free(current);
    queue->size--;
    return NOERROR;
}

bool queue_has(struct queue_t *queue, void *item){
    //verificação de segurança para ver se temos ponteiros nulo ou fila vazia
    if (queue == NULL || item == NULL || queue->size == 0){
        return false;
    }
    //criamos o ponteiro "viajante"
    struct queue_node_t *current = queue->head;
    //enquanto o viajante não aponta para o vazio
    while (current != NULL){
        if (current->item == item){
            return true;
        }
        current = current->next;
    }
    return false;
}

int queue_size(struct queue_t *queue){
    //testamos se a fila não existe
    if (queue == NULL){
        return ERROR;
    }
    //devolvemos o valor apontado do tamanho
    return queue->size;
}

void *queue_head(struct queue_t *queue){
    //primeiro testar se a fila está vazia
    if (queue == NULL || queue->size == 0){
        return NULL;
    }
    //iterator vai receber o head
    queue->iterator = queue->head;
    //devolvamemos o valor que o iterator aponta
    return queue->iterator->item;

}

void *queue_next(struct queue_t *queue){
    //primeiro testar se a fila está vazia, ou tamanho zero ou se o iterador aponta para nada
    if (queue == NULL || queue->size == 0 ||queue->iterator == NULL){
        return NULL;
    }
    queue->iterator = queue->iterator->next;
    //verificar se não caimos para fora da fila
    if (queue->iterator == NULL){
        return NULL;
    }
    //devolver a caixa azul
    return queue->iterator->item;
}

void *queue_item(struct queue_t *queue){
    //primeiro testar se a fila está vazia
    if (queue == NULL || queue->size == 0 || queue->iterator == NULL){
        return NULL;
    }
    //devolvemos o conteudo apontado de item
    return queue->iterator->item;
}

void queue_print(char *name, struct queue_t *queue, void(func)(void *)) {
    //se a fila não existe
    if (queue == NULL) {
        printf("%s: undef\n", name);
        return; //retorna vazio, pois a função é void
    }
    //começa a imprimir no formato exigido: "Nome: [ "
    printf("%s: [ ", name);
    //cria o ponteiro viajante no início da fila
    struct queue_node_t *current = queue->head;
    //percorre todos os itens da fila até cair no vazio
    while (current != NULL) {
        if (func != NULL) {
            func(current->item);
            printf(" "); // Imprime um espaço para não grudar com o próximo
        } else {
            printf("undef ");
        }
        //iteramos o ptr viajante
        current = current->next;
    }
    printf("] (%d items)\n", queue->size);
}

