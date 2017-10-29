#ifndef UART_H
#define UART_H

#include "driver.h"
#include "string.h"

/* commands recieved by Bluetooth */
#define SET 0x10
#define GET 0x20
#define DISP 0x30
#define TOGGLE 0x40
#define PING 0xFF

/* data to be set received by Bluetooth */
#define UART_TIME 0x01
#define UART_DATE 0x02

/* flags to be transmitted */
#define ALIVE 0x66
#define START_FLAG 0x7D

typedef enum {
	WAIT,
	CMD,
	SET_BT_SI,
	SET_BT_SJ,
	EXIT,
	END
} BT_states;

void UART_init(void);
void UART_commands_exec(volatile time_t* time_to_set);
bool set_BT_power_save (volatile time_t* curr_time);
bool UART_check_timeout (int32_t current_time_stamp);

#endif /* UART_H */
