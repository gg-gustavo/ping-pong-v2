// PingPongOS - PingPong Operating System
// Gustavo Gabriel Ripka GRR20203935
// Edison Luiz Matias Junior GRR20211790
// Gabriel Shigueo Ushiwa Kaguimoto Rodrigues GRR20221261

#include "ppos.h"

// tamanho total da heap: 64 MB
#define HEAP_SIZE 64*1024*1024

// numero maximo de blocos
#define MAX_BLOCKS 4094 

struct block_t {
    void* start;        // endereço inicial do bloco
    int size;           // tamanho do bloco em bytes
    int allocated;      // estado do bloco (livre ou alocado)
    int left;           // numero do bloco vizinho da esquerda
    int right;          // número do bloco vizinho da direita
};

static char heap[HEAP_SIZE];

static struct block_t blocks[MAX_BLOCKS];

int num_blocks;

void mem_init()
{
    unsigned long addr = (unsigned long)heap;
    int offset = 0;
    
    if (addr % 16 != 0)
        offset = 16 - (addr % 16);

    blocks[0].start = (void*)(heap + offset);
    
    int raw_size = HEAP_SIZE - offset;
    blocks[0].size = (raw_size / 16) * 16; 
    
    blocks[0].allocated = 0;
    blocks[0].left = 0;
    blocks[0].right = 0;
    
    num_blocks = 1;
}

// informa a quantidade de memória total, em bytes
int mem_size()
{
    return HEAP_SIZE;
}

// informa a quantidade de memória disponível, em bytes
int mem_avail()
{
    int curr = 0, avail = 0;

    do {
        if (blocks[curr].allocated == 0)
            avail += blocks[curr].size;
        curr = blocks[curr].right;
    } while (curr != 0);

    return avail;
}

void *mem_alloc(int size)
{
    int new_block = -1, curr = 0, idx = -1;

    if (size <= 0) return NULL;

    // ajusta o tamanho solicitado para um multiplo de 16 bytes
    size = ((size - 1) | 0x00000F) + 1;

    // procura um bloco com tamanho suficiente (first-fit)
    do {
        if (blocks[curr].allocated == 0 && size <= blocks[curr].size)
        {
            idx = curr;
            break;
        }
        curr = blocks[curr].right;
    } while (curr != 0);
    
    // se nao encontrar, retorna NULL (alocacao falhou)
    if (idx == -1)
        return NULL;

    void *start = blocks[idx].start;

    // se o tamanho do bloco livre encontrado for maior que o necessario,
    if (blocks[idx].size > size)
    {
        for (int i = 1; i < num_blocks; i++) {
            if (blocks[i].start == NULL && blocks[i].size == 0) {
                new_block = i;
                break;
            }
        }
        
        // Se não houver descritores mortos, cresce o num_blocks se houver espaço
        if (new_block == -1) {
            if (num_blocks < MAX_BLOCKS) {
                new_block = num_blocks;
                num_blocks++;
            } else {
                return NULL; // Esgotamento real de descritores
            }
        }
        
        blocks[new_block].start = (char *)start + size;
        blocks[new_block].size = blocks[idx].size - size;
        blocks[new_block].allocated = 0;
        blocks[new_block].left = idx;
        blocks[new_block].right = blocks[idx].right;

        blocks[blocks[idx].right].left = new_block;
        blocks[idx].right = new_block;
        blocks[idx].size = size;
    }

    // marca o bloco como ocupado
    blocks[idx].allocated = 1;

    // retorna um ponteiro para o inicio do bloco
    return start;
}

// libera um bloco de memória previamente alocado
// retorna NOERROR se ok ou ERROR se ptr for NULL ou inválido
int mem_free(void *ptr)
{
    int left_idx, right_idx, idx = -1;

    if (!ptr) return ERROR;

    // localiza o numero do bloco alocado a partir do endereço fornecido
    for (int i = 0; i < num_blocks; i++)
        if (blocks[i].start == ptr)
        {
            idx = i;
            break;
        }

    // se nao encontrar o bloco, retorna erro
    // se o bloco já estiver livre, retorna erro
    if (idx == -1 || blocks[idx].allocated == 0)
        return ERROR;

    // marca o bloco como livre
    blocks[idx].allocated = 0;

    // se o bloco a sua esquerda tambem estiver livre, funde-os em um unico bloco
    right_idx = blocks[idx].right;
    
    if (right_idx != idx && !blocks[right_idx].allocated &&
        ((char*)blocks[idx].start + blocks[idx].size == (char*)blocks[right_idx].start))
    {
        blocks[idx].size += blocks[right_idx].size;
        blocks[idx].right = blocks[right_idx].right;
        blocks[blocks[right_idx].right].left = idx;

        blocks[right_idx].size = 0;
        blocks[right_idx].start = NULL;
    }

    // se o bloco a sua direita tambem estiver livre, funde-os em um unico bloco
    left_idx = blocks[idx].left;

    if (left_idx != idx && !blocks[left_idx].allocated &&
        ((char*)blocks[left_idx].start + blocks[left_idx].size == (char*)blocks[idx].start))
    {
        blocks[left_idx].size += blocks[idx].size;
        blocks[left_idx].right = blocks[idx].right;
        blocks[blocks[idx].right].left = left_idx;
        
        blocks[idx].size = 0;
        blocks[idx].start = NULL;
    }

    return NOERROR;
}

// gera um relatório sobre o uso da memória
void mem_report()
{
    int curr, alloc_count, alloc_size, free_count, free_size;
    unsigned long addr_start, addr_end;

    curr = 0; alloc_count = 0; alloc_size = 0; free_count = 0; free_size = 0;

    do {
        if (blocks[curr].allocated) {
            alloc_count++;
            alloc_size += blocks[curr].size;
        } else {
            free_count++;
            free_size += blocks[curr].size;
        }
        
        curr = blocks[curr].right;
    } while (curr != 0);        

    printf("heap: %d KB allocated (%d blocks), %d KB free (%d blocks)\n",
        alloc_size / 1024, alloc_count, free_size / 1024, free_count);
    
    do {
        char *status = blocks[curr].allocated ? "aloc" : "FREE";
        addr_start = (unsigned long)blocks[curr].start;
        addr_end = addr_start + blocks[curr].size - 1;
        
        printf("heap: block %6d: %p - %p %s prev %6d next %6d size %d\n",
               curr, (void*)addr_start, (void*)addr_end, status,
               blocks[curr].left, blocks[curr].right, blocks[curr].size);
    
        curr = blocks[curr].right;
    } while (curr != 0);
}