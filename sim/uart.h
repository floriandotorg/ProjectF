#ifndef UART_H
#define UART_H

#include <stdint.h>

struct uart_sturct;
typedef struct uart_sturct uart_t;

uart_t* uart_create();
uart_t* uart_free(uart_t* uart);

void uart_write_send(uart_t* uart, uint8_t val);
void uart_write_control(uart_t* uart, uint8_t val);

uint8_t uart_read_status(uart_t* uart);
uint8_t uart_read_recv(uart_t* uart);

int uart_recv_loop(uart_t* uart, uint8_t *interrupt_flags);

/*
int main()
{
    uint8_t interrupt_flags = 0, uart_status = 0;
    uart_t *uart = uart_create();
    
    uart_write_control(uart, 0xff);
    
    uart_write_send(uart, 0x0d);
    
    for(;;)
    {
        uart_recv_loop(uart, &interrupt_flags);
        if(interrupt_flags != 0)
        {
            uart_status = uart_read_status(uart);
            
            if(uart_status & (1<<0))
            {
                printf("recv: %c\n", uart_read_recv(uart));
            }
            else
            {
                printf("interrrupt uart status: %i\n", uart_status);
            }
            
            interrupt_flags = 0;
        }
    }
    
    uart = uart_free(uart);
    
    return 0;
}
*/

#endif
