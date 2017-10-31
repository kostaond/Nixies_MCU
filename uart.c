#include "uart.h"

//#define BAUD_RATE 115200
#define BAUD_RATE 57600

#define UART_RB_SIZE 32
#define UART_MSG_SIZE 6
#define RX_TIMEOUT 2 /* 2 seconds */

#define BT_RESP_SIZE 5

/* Transmit and receive ring buffers */
STATIC RINGBUFF_T rxring, txring;
/* Ring buffer size */
static uint8_t UART_data_RX[UART_RB_SIZE];
static uint8_t UART_data_TX[UART_RB_SIZE];
/* Variables to indicate stale RX */
volatile bool RX_new_data = false;


void UART_init (void)
{
	RingBuffer_Init(&rxring, UART_data_RX, 1, UART_RB_SIZE);
	RingBuffer_Init(&txring, UART_data_TX, 1, UART_RB_SIZE);
	
	Chip_Clock_SetUARTClockDiv(1);
	
	/* Setup UART */
	Chip_UART_Init(LPC_USART0);
	Chip_UART_ConfigData(LPC_USART0, UART_CFG_DATALEN_8 | UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1 );
	Chip_Clock_SetUSARTNBaseClockRate((BAUD_RATE * 16), true);
	Chip_UART_SetBaud(LPC_USART0, BAUD_RATE);
	Chip_UART_Enable(LPC_USART0);
	Chip_UART_TXEnable(LPC_USART0);
	
	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(LPC_USART0, UART_INTEN_RXRDY);
	//Chip_UART_IntDisable(LPC_USART0, UART_INTEN_RXRDY);
	Chip_UART_IntDisable(LPC_USART0, UART_INTEN_TXRDY);	/* May not be needed */
	
	NVIC_EnableIRQ(UART0_IRQn);
}


void UART0_IRQHandler (void)
{
	if((Chip_UART_GetStatus(LPC_USART0) & UART_STAT_RXRDY) != 0)
	{
		RX_new_data = true;
	}
	Chip_UART_IRQRBHandler(LPC_USART0, &rxring, &txring);
}

bool set_BT_power_save (volatile time_t* curr_time)
{
  static BT_states state = CMD;
	static BT_states prev_state;
	
	bool setting_done = false;
	char rx_data[BT_RESP_SIZE + 1]; /* +1 do add null terminator */
	char *tx_data;
	int32_t curr_time_stamp;
	
	switch (state)
	{
		case CMD:
			tx_data = "$$$";
		  Chip_UART_SendRB(LPC_USART0, &txring, tx_data, 3); /* actual size of tx_data is 4, therefore -1 because we don't want to send null terminator */
			prev_state = state;
			state = WAIT;
		break;
			
		case WAIT:
			curr_time_stamp = curr_time->seconds + curr_time->minutes*60 + curr_time->hours*360;
			if (RingBuffer_GetCount(&rxring) == BT_RESP_SIZE)
			{
				Chip_UART_IntDisable(LPC_USART0, UART_INTEN_RXRDY); /* disable RX interrupt to protect integrity of rxring buffer during reading */	
				Chip_UART_ReadRB(LPC_USART0, &rxring, rx_data, BT_RESP_SIZE);
				Chip_UART_IntEnable(LPC_USART0, UART_INTEN_RXRDY);
				rx_data[BT_RESP_SIZE] = '\0';
				if ((strcmp(rx_data, "CMD\r\n") == 0) || (strcmp(rx_data, "AOK\r\n") == 0) || (strcmp(rx_data, "END\r\n") == 0))
				{
					prev_state++;
					state = prev_state;
				}
				else
				{
					/* message was not sucessfull */
					state = (prev_state == EXIT) ? END : EXIT; /* if previous state was EXIT, then END, otherwise try to exit set mode wit waiting for "END\r\n" sent by BT*/
				}					
			}
			if (UART_check_timeout(curr_time_stamp))
			{
				state = (prev_state == EXIT) ? END : EXIT;
			}
		break;
			
		case SET_BT_SI:
			tx_data = "SI,0012\r"; /* set Discover window to the lowest value */
			Chip_UART_SendRB(LPC_USART0, &txring, tx_data, 8);
			prev_state = state;
			state = WAIT;
		break;
		
		case SET_BT_SJ:
			tx_data = "SJ,0012\r"; /* set Connect window to the lowest value */
			Chip_UART_SendRB(LPC_USART0, &txring, tx_data, 8);
			prev_state = state;
			state = WAIT;
		break;
		
		case EXIT:
			tx_data = "---\r\n";
			Chip_UART_SendRB(LPC_USART0, &txring, tx_data, 5);	
			prev_state = state;
			state = WAIT;
		break;
		
		case END:
			setting_done = true;
			RingBuffer_Flush(&rxring);
			RingBuffer_Flush(&txring);		
		break;
		
		default:
			state = END;
	}
		
	return setting_done;
}

void UART_commands_exec(volatile time_t* time_to_set, volatile display_t* user_data_to_set)
{
	uint8_t data[UART_MSG_SIZE];
	int32_t curr_time_stamp;
	
	/* need to combine ss/mm/hh to adress corner cases when time is 20:59, for example. If stored only seconds,
	timeout difference checked below would be incorrect (01 - 59)*/
	curr_time_stamp = time_to_set->seconds + time_to_set->minutes*60 + time_to_set->hours*360;
	
	while (RingBuffer_GetCount(&rxring) >= UART_MSG_SIZE)
	{				
		Chip_UART_IntDisable(LPC_USART0, UART_INTEN_RXRDY); /* disable RX interrupt to protect integrity of rxring buffer during reading */
		Chip_UART_ReadRB(LPC_USART0, &rxring, data, UART_MSG_SIZE);
		Chip_UART_IntEnable(LPC_USART0, UART_INTEN_RXRDY);		
		
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
			
			case (GET | UART_TIME):
				data[0] = START_FLAG;
				data[1] = UART_TIME;
				data[2] = time_to_set->hours;
				data[3] = time_to_set->minutes;
				data[4] = time_to_set->seconds;
				data[5] = 0;
				Chip_UART_SendRB(LPC_USART0, &txring, data, UART_MSG_SIZE);
			break;
		
			case (GET | UART_DATE):
				data[0] = START_FLAG;
				data[1] = UART_DATE;
				data[2] = time_to_set->days;
				data[3] = time_to_set->months;
				data[4] = time_to_set->years >> 8;
				data[5] = time_to_set->years & 0xFF;
				Chip_UART_SendRB(LPC_USART0, &txring, data, UART_MSG_SIZE);
			break;
			
			case (DISP):
				time_to_set->change_display_timeout = 0;
				time_to_set->curr_displayed = USER_DATA;
				user_data_to_set->hours = data[2];
				user_data_to_set->minutes = data[3];
				user_data_to_set->seconds = data[4];				
			break;
			
			case (TOGGLE):
				if (time_to_set->curr_displayed == DATE)
				{
					time_to_set->curr_displayed = TIME | LOCK;
					time_to_set->change_display_timeout = 0;
				}
				else if (time_to_set->curr_displayed == TIME)
				{
					time_to_set->curr_displayed = DATE | LOCK;
					time_to_set->change_display_timeout = 0;
				}
			break;				
		}
	}
	
	UART_check_timeout(curr_time_stamp);
}

bool UART_check_timeout (int32_t current_time_stamp)
{
	static int32_t time_stamp = 0;
	
	if (RX_new_data)
	{	
		time_stamp = current_time_stamp;
		RX_new_data = false;
	} 
	else if ((RingBuffer_GetCount(&rxring) > 0) &&
		((current_time_stamp - time_stamp) >= RX_TIMEOUT))
	{
		 /*if timeout, flush the incomplete rx ring buffer to have it clean for next incoming messages if connection is established again */
		Chip_UART_IntDisable(LPC_USART0, UART_INTEN_RXRDY);	/* disable RX interrupt to protect integrity of rxring buffer during flushing */
		RingBuffer_Flush(&rxring);
		Chip_UART_IntEnable(LPC_USART0, UART_INTEN_RXRDY);

		return true;
	}
	
	return false;
}
