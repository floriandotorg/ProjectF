#include "uart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __MINGW32__
#include <conio.h>
#else
#include <termios.h>
static int kbhit() {
   struct termios term, oterm;
   int fd = 0;
   int c = 0;
   tcgetattr(fd, &oterm);
   memcpy(&term, &oterm, sizeof(term));
   term.c_lflag = term.c_lflag & (!ICANON);
   term.c_cc[VMIN] = 0;
   term.c_cc[VTIME] = 1;
   tcsetattr(fd, TCSANOW, &term);
   c = getchar();
   tcsetattr(fd, TCSANOW, &oterm);
   if (c != -1)
    ungetc(c, stdin);
   return ((c != -1) ? 1 : 0);
}

static int getch()
{
   static int ch = -1, fd = 0;
   struct termios neu, alt;
   fd = fileno(stdin);
   tcgetattr(fd, &alt);
   neu = alt;
   neu.c_lflag &= ~(ICANON|ECHO);
   tcsetattr(fd, TCSANOW, &neu);
   ch = getchar();
   tcsetattr(fd, TCSANOW, &alt);
   return ch;
}
#endif

struct uart_sturct
{
    uint8_t status;
    uint8_t control;
    uint8_t recv;
    uint8_t recv_empty;
};

uart_t* uart_create()
{
    uart_t *uart = malloc(sizeof(*uart));
    memset(uart,0,sizeof(*uart));
    return uart;
}

void uart_write_send(uart_t* uart, uint8_t val)
{
    // uart tx enabled
    if(uart->control & (1<<1))
    {
        putc(val, stdout);
        uart->status |= (1<<1);
    }
}

void uart_write_control(uart_t* uart, uint8_t val)
{
    uart->control = val;
}

uint8_t uart_read_status(uart_t* uart)
{
    const uint8_t status = uart->status;
    uart->status = 0;
    return status;
}

uint8_t uart_read_recv(uart_t* uart)
{
    // reset receive complete
    uart->status &= ~(1<<0);
    uart->recv_empty = 1;
    return uart->recv;
}

int uart_recv_loop(uart_t* uart, uint8_t *interrupt_flags)
{
    // uart rx enabled and keyboard hit
    if(uart->control & (1<<0) &&  kbhit())
    {
        uart->recv = getch();
        uart->status |= (1<<0);
        
        // data over run error 
        if(!uart->recv_empty)
        {
            uart->status |= (1<<2) | (1<<3);
        }
        
        // rx interrupt enabled
        if(uart->control & (1<<0))
        {
            *interrupt_flags |= (1<<0);
        }
    }

    // tx interrupt enabled and tx completed
    if(uart->control & (1<<1) && uart->status & (1<<1))
    {
        *interrupt_flags |= (1<<0);
    }
    
    return (*interrupt_flags &(1<<0)) != 0;
}

uart_t* uart_free(uart_t* uart)
{
    free(uart);
    return NULL;
}
