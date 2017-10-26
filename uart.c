#include "uart.h"


/* Transmit and receive ring buffers */
STATIC RINGBUFF_T rxring, txring;
/* Ring buffer size */
#define UART_RB_SIZE 32
#define UART_MSG_SIZE 6
static uint8_t UART_data_RX[UART_RB_SIZE];
static uint8_t UART_data_TX[UART_RB_SIZE];


void UART0_IRQHandler (void)
{
	Chip_UART_IRQRBHandler(LPC_USART0, &rxring, &txring);
}

void UART_init (void)
{
	RingBuffer_Init(&rxring, UART_data_RX, 1, UART_RB_SIZE);
	RingBuffer_Init(&txring, UART_data_TX, 1, UART_RB_SIZE);
	
	Chip_Clock_SetUARTClockDiv(1);
	
	/* Setup UART */
	Chip_UART_Init(LPC_USART0);
	Chip_UART_ConfigData(LPC_USART0, UART_CFG_DATALEN_8 | UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1 );
	Chip_Clock_SetUSARTNBaseClockRate((115200 * 16), true);
	Chip_UART_SetBaud(LPC_USART0, 115200);
	Chip_UART_Enable(LPC_USART0);
	Chip_UART_TXEnable(LPC_USART0);
	
	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(LPC_USART0, UART_INTEN_RXRDY);
	//Chip_UART_IntDisable(LPC_USART0, UART_INTEN_RXRDY);
	Chip_UART_IntDisable(LPC_USART0, UART_INTEN_TXRDY);	/* May not be needed */
	
	NVIC_EnableIRQ(UART0_IRQn);
}

void UART_commands_exec(volatile time_t* time_to_set)
{
	uint8_t data[UART_MSG_SIZE];
	if (RingBuffer_GetCount(&rxring) >= UART_MSG_SIZE)
	{			
		Chip_UART_ReadRB(LPC_USART0, &rxring, data, UART_MSG_SIZE);	
		switch (data[1])
		{
			case (SET | UART_TIME):
				time_to_set->seconds = data[4];
				time_to_set->minutes = data[3];
				time_to_set->hours = data[2];
			break;
		
			case (SET | UART_DATE):
				time_to_set->days = data[2];
				time_to_set->months = data[3];
				time_to_set->years = data[4] << 8 | data[5];
			break;
			
			case (PING):
				memset(data, 0x0, sizeof(data));
				data[0] = START_FLAG;
				data[1] = ALIVE;
				Chip_UART_SendRB(LPC_USART0, &txring, data, UART_MSG_SIZE);
			break;
			
			case (TOGGLE):
			  if (time_to_set->curr_displayed == DATE)
				{
					time_to_set->curr_displayed = TIME | LOCK;
					time_to_set->change_display_timeout = 0;
				}
				else
				{
					time_to_set->curr_displayed = DATE | LOCK;
					time_to_set->change_display_timeout = 0;
				}
			break;				
		}
	}
}