#include "driver.h"

#define SYSTICKRATE_HZ 1

const uint32_t OscRateIn = 18432000ul;
const uint32_t ExtRateIn = 0;

uint32_t SysTick_Config_half(uint32_t ticks)
{
  if ((ticks - 1UL) > SysTick_LOAD_RELOAD_Msk)
  {
    return (1UL);                                                   /* Reload value impossible */
  }

  SysTick->LOAD  = (uint32_t)(ticks - 1UL);                         /* set reload register */
  NVIC_SetPriority (SysTick_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL); /* set Priority for Systick Interrupt */
  SysTick->VAL   = 0UL;                                             /* Load the SysTick Counter Value */
  SysTick->CTRL  = /*SysTick_CTRL_CLKSOURCE_Msk |*/
                   SysTick_CTRL_TICKINT_Msk   |
                   SysTick_CTRL_ENABLE_Msk;                         /* Enable SysTick IRQ and SysTick Timer */
  return (0UL);                                                     /* Function successful */
}

void setupMRT(uint8_t ch, MRT_MODE_T mode, uint32_t rate)
{
	LPC_MRT_CH_T *pMRT;

	/* Get pointer to timer selected by ch */
	pMRT = Chip_MRT_GetRegPtr(ch);

	/* Setup timer with rate based on MRT clock */
	Chip_MRT_SetInterval(pMRT, (Chip_Clock_GetSystemClockRate() / rate) |
						 MRT_INTVAL_LOAD);

	/* Timer mode */
	Chip_MRT_SetMode(pMRT, mode);

	/* Clear pending interrupt and enable timer */
	Chip_MRT_IntClear(pMRT);
	Chip_MRT_SetEnabled(pMRT);
}

void board_init (void)
{
	/* Set port 0 pins 1, 4, 7, 10, 11, 13, 14, 15, 16 and 17 to the output direction*/
	Chip_GPIO_SetPortDIROutput(LPC_GPIO_PORT, 0, OUT_PORT_MASK);
	
	/* Set port 0 pins 12 nad 5 to be input direction */
	Chip_GPIO_SetPortDIRInput(LPC_GPIO_PORT, 0, IN_PORT_MASK);
	
	/* Connect the UART TX/RX signals to port pins 6 and 0) */
	Chip_SWM_DisableFixedPin(SWM_FIXED_ACMP_I1);
	Chip_SWM_MovablePinAssign(SWM_U0_TXD_O, TX_PIN);
	Chip_SWM_MovablePinAssign(SWM_U0_RXD_I, RX_PIN);
	
	/* Set GPIO port mask value to make sure only port 0
	are active during state change */
	Chip_GPIO_SetPortMask(LPC_GPIO_PORT, 0, ~OUT_PORT_MASK);
	
	/* Set all out pins to 0 */
	Chip_GPIO_SetMaskedPortValue(LPC_GPIO_PORT, 0, 0);
	
	/* Set Glitch filter for switchs*/
	Chip_Clock_SetIOCONCLKDIV(IOCONCLKDIV1, 250);
	Chip_IOCON_PinSetSampleMode(LPC_IOCON, IOCON_PIO12, PIN_SMODE_CYC3); //SW1 
	Chip_IOCON_PinSetClockDivisor(LPC_IOCON, IOCON_PIO12, IOCONCLKDIV1);
	Chip_IOCON_PinSetSampleMode(LPC_IOCON, IOCON_PIO5, PIN_SMODE_CYC3); //SW2
	Chip_IOCON_PinSetClockDivisor(LPC_IOCON, IOCON_PIO5, IOCONCLKDIV1);
	
	/* Disable !RESET pin */
	Chip_SWM_DisableFixedPin(SWM_FIXED_RST);
	
	/* Enable SysTick Timer */
	SysTick_Config_half((OscRateIn/2) / SYSTICKRATE_HZ);
}


void set_number (uint8_t number)
{	
#ifdef BOARD_REV1
	work_around_7442_4028(&number);
#endif
	
	Chip_GPIO_SetPinState (LPC_GPIO_PORT, 0, BCD_A, number & 0x01);
	Chip_GPIO_SetPinState (LPC_GPIO_PORT, 0, BCD_B, number & 0x02);
	Chip_GPIO_SetPinState (LPC_GPIO_PORT, 0, BCD_C, number & 0x04);
	Chip_GPIO_SetPinState (LPC_GPIO_PORT, 0, BCD_D, number & 0x08);
}

void work_around_7442_4028 (uint8_t* number)
{
	switch(*number)
	{
		case 0:
			*number = 4;
			break;
		case 1:
			*number = 2;
			break;
		case 2:
			*number = 0;
			break;			
		case 3:
			*number = 7;
			break;
		case 4:
			*number = 9;
			break;
		case 5:
			*number = 5;
			break;
		case 6:
			*number = 6;
			break;
		case 7:
			*number = 8;
			break;
		case 8:
			*number = 1;
			break;
		case 9:
			*number = 3;
			break;
	}
}

uint8_t days_in_month (uint8_t month, uint16_t year)
{
	uint8_t days;
	
	if (month != 2)
	{
		if (((month < 7) && !(month % 2)) || ((month > 7) && (month % 2))) 
		{
			days = 30;
		}
		else
		{
			days = 31;
		}
	}
	else if (year % 4) /* leap year */
	{
		days = 28;
	}
	else 
	{
		days = 29;
	}
	
	return days;
}

void time_inc_dec(volatile time_t* time, int8_t dec_inc_value, date_time what)	
{	
	bool time_only;
	
	time_only = what & TIME_ONLY;
	what &= ~TIME_ONLY;
	
	switch (what)
	{
		case SECONDS:
			time->seconds += dec_inc_value;
			break;
		case MINUTES:
			time->minutes += dec_inc_value;
			break;
		case HOURS:
			time->hours += dec_inc_value;
			break;
		
		case DAYS:
			time->days += dec_inc_value;
			break;
		case MONTHS:
			time->months += dec_inc_value;
			break;
		case YEARS:
			time->years += dec_inc_value;
			break;
	}		
		
	if (dec_inc_value > 0)
	{			
		if (time->seconds > 59)
		{
			time->minutes++;
			time->seconds = 0;
		}
		
		if (time->minutes > 59)
		{
			time->hours++;
			time->minutes = 0;
		}
		
		if (time->hours > 23)
		{
			if (!time_only)
			{
				time->days++;
			}
			time->hours = 0;
		}
		
		if (time->days > days_in_month(time->months, time->years))
		{
			time->months++;
			time->days = 1;
		}
		
		if (time->months > 12)
		{
			time->years++;
			time->months = 1;
		}	
	}
	else
	{
		if (time->seconds > 255)
		{
			time->minutes--;
			time->seconds = 59;
		}
		
		if (time->minutes == 255)
		{
			time->hours--;
			time->minutes = 59;
		}
		if (time->hours == 255)
		{
			time->hours = 23;
		}
		
		if (time->days == 0)
		{
			time->months--;
			time->days = days_in_month(time->months, time->years);
		}
		
		if (time->months == 0)
		{
			time->years--;
			time->months = 12;
			
			if (time->years < 2000)
			{
				time->years = 2000;
			}
		}
	}
}	


void roll_numbers(volatile time_t* time, volatile display_t* user_data, volatile display_t *display)	
{
	static uint8_t over_date = 0;
	static uint8_t over_time = 0;
	static uint8_t over_user = 0;
	static bool one_time_roll = FALSE;
	bool finish = FALSE;
	
	if ((time->curr_displayed & ~LOCK) == DATE)/* mask LOCK since we don't care */
	{
		finish = roll(&display->seconds, to_BCD(year_to_number(time->years)), over_date);
		finish &= roll(&display->minutes, to_BCD(time->months), over_date);
		finish &= roll(&display->hours, to_BCD(time->days), over_date);

		if (finish)
		{
			one_time_roll = TRUE;
			over_time = 0;
			over_user = 0;
			
			time->curr_displayed &= ~LOCK; /* unlock */
		}	
		else
		{
			over_date++; /* to roll over all 10 (0-9) digits */
		}
	}

	if ((time->curr_displayed & ~LOCK) == TIME) /* time */
	{
		if (one_time_roll)
		{
			finish = roll(&display->seconds, to_BCD(time->seconds), over_time);
			finish &= roll(&display->minutes, to_BCD(time->minutes), over_time);
			finish &= roll(&display->hours, to_BCD(time->hours), over_time);
			
			if (finish)
			{
				one_time_roll = FALSE; /* prevents rolling seconds when they are changed under normal conditions */ 
				over_date = 0;
				over_user = 0;
				
				time->curr_displayed &= ~LOCK; /* unlock */
			}
			else
			{
				over_time++; /* to roll over all 10 (0-9) digits */
			}
		}
		else
		{
			display->seconds = to_BCD(time->seconds);
			display->minutes = to_BCD(time->minutes);
			display->hours = to_BCD(time->hours);
		}
	}
	
	if ((time->curr_displayed & ~LOCK) == USER_DATA)
	{
		finish = roll(&display->seconds, to_BCD(user_data->seconds), over_user);
		finish &= roll(&display->minutes, to_BCD(user_data->minutes), over_user);
		finish &= roll(&display->hours, to_BCD(user_data->hours), over_user);
		
		if (finish)
		{
			one_time_roll = TRUE;
			over_date = 0;
			over_time = 0;
			
			time->curr_displayed &= ~LOCK; /* unlock */
		}
		else
		{
			over_user++; /* to roll over all 10 (0-9) digits */
		}
	}
}

bool roll(volatile uint8_t* displayed, uint8_t needed, uint8_t over)	
{
	bool done = TRUE;
	
	/* end when new value to display found but continue rolling if not all combinations were iluminated */
	if (((*displayed & 0x0F) != (needed & 0x0F)) || (over <= 9)) 
	{
		*displayed += 1;
		if ((*displayed & 0x0F) > 0x09)
		{
			*displayed &= 0xF0;
		}
		done = FALSE;
	}
			
	if (((*displayed & 0xF0) != (needed & 0xF0)) || (over <= 9))
	{
		*displayed += 0x10;
		if ((*displayed & 0xF0) > 0x90)
		{
			*displayed &= 0x0F;				
		}
		done = FALSE;
	}
	
	return done;
}

uint8_t to_BCD (uint8_t number)
{
	return ((number / 10) << 4) | (number % 10);
}

uint8_t year_to_number (uint16_t year)
{
	return year - 2000;
}
