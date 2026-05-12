// PingPongOS - PingPong Operating System
// Gustavo Gabriel Ripka GRR20203935
// Edison Luiz Matias Junior GRR20211790
// Gabriel Shigueo Ushiwa Kaguimoto Rodrigues GRR20221261

#include "hardware/cpu.h"
#include "task.h"
#include "dispatcher.h"
#include "time.h"

// Variável global para rastrear o tempo do sistema
unsigned int system_time = 0;

void tick_handler(int signum)
{
    extern struct task_t *current_task;
    
    // Incrementa o contador de ticks do sistema
    system_time++;
    
    // Decrementa o quantum da tarefa atual se for uma tarefa de usuário
    if (current_task != NULL && current_task->quantum_remaining > 0)
    {
        current_task->quantum_remaining--;
        
        // Se o quantum expirou, faz preempção
        if (current_task->quantum_remaining == 0)
        {
            // Volta para o dispatcher
            task_yield();
        }
    }
}

void time_init()
{
    system_time = 0;
    hw_irq_handle(IRQ_TIMER, tick_handler);
    hw_timer(1, 1);
}

int systime()
{
    return system_time;
}