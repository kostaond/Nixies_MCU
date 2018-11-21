#ifndef DRIVER_H
#define DRIVER_H

#define CORE_M0PLUS
#include "chip.h"
#include "system_LPC8xx.h"


typedef struct time {
	uint8_t  seconds;
	uint8_t  minutes;
	uint8_t  hours;
	uint8_t  days;
	uint8_t  months;
	uint16_t years;
	uint8_t	curr_displayed;
	uint16_t change_display_timeout;
	uint16_t show_time;
	uint16_t show_date;
	uint16_t show_user_data;
} time_t;

typedef struct display {
	uint8_t  seconds;
	uint8_t  minutes;
	uint8_t  hours;
	uint8_t	pad;
} display_t;

typedef enum {
	SECONDS,
	MINUTES,
	HOURS,
	DAYS,
	MONTHS,
	YEARS
} date_time;


/* Nixie clock board constants */

/* Oscillator setting - data needed for LPC8xx Clock Driver functions of LPCOpen*/
/**
 * @brief	System oscillator rate
 * This value is defined externally to the chip layer and contains
 * the value in Hz for the external oscillator for the board. If using the
 * internal oscillator, this rate can be 0.
 */
//extern const uint32_t OscRateIn = 18432000ul;

/**
 * @brief	Clock rate on the CLKIN pin
 * This value is defined externally to the chip layer and contains
 * the value in Hz for the CLKIN pin for the board. If this pin isn't used,
 * this rate can be 0.
 */
//extern const uint32_t ExtRateIn = 0;

/* --------------------------- */

/* I/O port pin layout definition */
#define SEC 					13
#define SEC_TENS 			17

#define MINUT 				15
#define MINUT_TENS 		1

#define HOUR					7
#define HOUR_TENS 		14

#define SW1						12
#define SW2						5

#ifdef BOARD_REV1
	#define TX_PIN				0
	#define RX_PIN				6
#else
	#define TX_PIN				6
	#define RX_PIN				0
#endif

#ifdef BOARD_REV1
	#define BCD_A				16
	#define BCD_B				11
	#define BCD_C				4
	#define BCD_D				10
#else
	#define BCD_A				4
	#define BCD_B				16
	#define BCD_C				10
	#define BCD_D				11
#endif

/* ----------------------------- */

#define OUT_PORT_MASK 0x3EC92 /* pins 1, 4, 7, 10, 11, 13, 14, 15, 16 and 17 */
#define IN_PORT_MASK (1 << SW1) | (1 << SW2)


#define	TIME	0
#define	DATE	1
#define USER_DATA 2
#define	LOCK 	0x80

#define TIME_ONLY 0x80

/* Functions definitions */
void setupMRT(uint8_t ch, MRT_MODE_T mode, uint32_t rate);
void set_number (uint8_t number);
void work_around_7442_4028 (uint8_t* number);
uint32_t SysTick_Config_half(uint32_t ticks);
void board_init (void);
void time_inc_dec (volatile time_t* time, int8_t dec_inc_value, date_time what);
void roll_numbers(volatile time_t* time, volatile display_t* user_data, volatile display_t *display);
uint8_t to_BCD (uint8_t number);
bool roll(volatile uint8_t* displayed, uint8_t needed, uint8_t over);
uint8_t year_to_number (uint16_t year);
uint8_t days_in_month (uint8_t month, uint16_t year);

#endif /* DRIVER_H */
