#include "driver.h"
#include "uart.h"

//#include "stdio.h"
#include "string.h"

#define INC_START_RATE 3 /* rate at which start when setting button is continuously pushed down */ 
#define INC_MAX_RATE 100 /* max rate of increasing/decreasing minutes/days when setting new time */
#define REFRESH_RATE 1000 /* refresh rate for multiplexing 1 ms = 1000 Hz */
#define BLANK_RATE 10000 /* blanking interval 100 us = 10000 (2*100 us; first turn off anode, wait 100us, set cathodes, wait 100us, turn on anode */
#define ROLL_RATE 15 /* roll numbers in 15 Hz when changing from TIME to DATE */
#define LEAVE_SET_MODE_IN 4 /* leave set mode in 4 seconds when no button is pushed */

#define SHOW_TIME 90 /* Show time for 90 seconds */
#define SHOW_DATE 10 /* Show date for 10 seconds */

#define NOT_IN_SET_MODE 0
#define PRE_SET_MODE 1
#define SET_MODE_BLINK	2
#define SET_MODE_INC	3

volatile time_t my_time;
volatile uint8_t anode_ON = 0;
volatile uint8_t set_mode = NOT_IN_SET_MODE;
volatile display_t to_display;
volatile bool blink = FALSE;
volatile int8_t set_value;
volatile uint8_t leave_set_mode = 0;

void SysTick_Handler(void)
{
	switch (set_mode)
	{
		case NOT_IN_SET_MODE:
			time_inc_dec(&my_time, +1, SECONDS);
			blink = FALSE;
			
			my_time.change_display_timeout++;		
			if ((my_time.curr_displayed == TIME) && (my_time.change_display_timeout > SHOW_TIME)) /* LOCK equals 0 => unlocked, if locked, there is no change between date & time */
			{
				my_time.curr_displayed = DATE | LOCK;
				my_time.change_display_timeout = 0;
			}
			else if ((my_time.curr_displayed == DATE) && (my_time.change_display_timeout > SHOW_DATE))
			{
				my_time.curr_displayed = TIME | LOCK;
				my_time.change_display_timeout = 0;
			}
			break;
			
		case PRE_SET_MODE:
			blink ^= TRUE;
			time_inc_dec(&my_time, +1, SECONDS);
			break;
		
		case SET_MODE_BLINK:
			blink ^= TRUE;
			time_inc_dec(&my_time, +1, SECONDS);
			leave_set_mode++;
			
			if (leave_set_mode > LEAVE_SET_MODE_IN) /* leave set mode after elapsing X sec */
			{
				leave_set_mode = 0;
				
				set_mode = NOT_IN_SET_MODE;
				
				my_time.change_display_timeout = 0;
				
				/* disable interrupt for +/- buttons handlers */
				NVIC_DisableIRQ(PININT0_IRQn);
				NVIC_DisableIRQ(PININT1_IRQn);
				
				/* Clear pending IRQs for buttons */
				NVIC_ClearPendingIRQ(PININT2_IRQn);
				
				/* enable interrupt for change displayed data (time/date) */
				NVIC_EnableIRQ(PININT2_IRQn);
			}
			break;
			
		case SET_MODE_INC:
			leave_set_mode = 0;
			break;
	}
}
volatile bool turn_anode_on = FALSE;
void MRT_IRQHandler(void)
{
	uint32_t int_pend;
	uint32_t interval_val;
	
	/* Get interrupt pending status for all timers */
	int_pend = Chip_MRT_GetIntPending();
	Chip_MRT_ClearIntPending(int_pend);

	if (set_mode == NOT_IN_SET_MODE)
	{		
		/* Channel 3 - speed of numbers rolling*/
		if (int_pend & MRTn_INTFLAG(3))
		{
			roll_numbers(&my_time, &to_display);	
		}
	}
	else if (my_time.curr_displayed == TIME) /* if in SET MODE and time to be displayed */
	{
		to_display.seconds = to_BCD(my_time.seconds);
		to_display.minutes = to_BCD(my_time.minutes);
		to_display.hours = to_BCD(my_time.hours);
	}
	else /* in SET MODE and date to be displayed */
	{
		to_display.seconds = to_BCD(year_to_number(my_time.years));
		to_display.minutes = to_BCD(my_time.months);
		to_display.hours = to_BCD(my_time.days);
	}
	
	/* Channel 0 - base period for multiplexing */
	if (int_pend & MRTn_INTFLAG(0)) 
	{
		/* Enable timer 1 in single one mode to limit blanking interval */
		setupMRT(1, MRT_MODE_ONESHOT, BLANK_RATE);
		turn_anode_on = FALSE;
		
		/* Blanking interval */
		Chip_GPIO_SetPinOutLow(LPC_GPIO_PORT, 0, SEC_TENS);
		Chip_GPIO_SetPinOutLow(LPC_GPIO_PORT, 0, SEC);
		Chip_GPIO_SetPinOutLow(LPC_GPIO_PORT, 0, MINUT_TENS);
		Chip_GPIO_SetPinOutLow(LPC_GPIO_PORT, 0, MINUT);
		Chip_GPIO_SetPinOutLow(LPC_GPIO_PORT, 0, HOUR_TENS);
		Chip_GPIO_SetPinOutLow(LPC_GPIO_PORT, 0, HOUR);
			
		anode_ON++;
		if (anode_ON > 5)
		{
			anode_ON = 0;
		}	
	}

	/* Channel 1 is single shot - limits blanking interval */
	if ((int_pend & MRTn_INTFLAG(1)) && !blink) 
	{
		if (turn_anode_on == FALSE)
		{
			/* Enable timer 1 in single one mode to limit blanking interval */
			setupMRT(1, MRT_MODE_ONESHOT, BLANK_RATE);
			turn_anode_on = TRUE;
			
			/* Set number (cathode) for the nixie anode which will be turned ON in next interrupt */ 
			switch(anode_ON)
			{
				case 0:
					set_number(to_display.seconds & 0x0F);
				break;
				case 1:
					set_number(to_display.seconds >> 4);
				break;
				case 2:
					set_number(to_display.minutes & 0x0F);
				break;
				case 3:
					set_number(to_display.minutes >> 4);
				break;
				case 4:
					set_number(to_display.hours & 0x0F);
				break;
				case 5:
					set_number(to_display.hours >> 4);
				break;
			}		
		}
		else /* turn anode ON */
		{
			turn_anode_on = FALSE;
			switch(anode_ON)
			{
				case 0:
					Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 0, SEC);
				break;
				case 1:
					 Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 0, SEC_TENS);
				break;
				case 2:
					Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 0, MINUT);
				break;
				case 3:
					Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 0, MINUT_TENS);
				break;
				case 4:
					Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 0, HOUR);
				break;
				case 5:
					Chip_GPIO_SetPinOutHigh(LPC_GPIO_PORT, 0, HOUR_TENS);
				break;
			}
		}
	}
	
	/* Channel 2 - to enter setting and to increase setting speed when in set mode*/
	if (int_pend & MRTn_INTFLAG(2))		
	{			
		switch(set_mode) 
		{
			case SET_MODE_INC:
				if (my_time.curr_displayed == TIME)
				{
					time_inc_dec(&my_time, set_value, MINUTES | TIME_ONLY);
				}
				else
				{
					time_inc_dec(&my_time, set_value, DAYS);
				}
				
				interval_val = Chip_MRT_GetInterval(Chip_MRT_GetRegPtr(2));
				interval_val -= interval_val/6;
				
				if (interval_val < (Chip_Clock_GetSystemClockRate() / INC_MAX_RATE))
				{
					interval_val = (Chip_Clock_GetSystemClockRate() / INC_MAX_RATE);
				}
				Chip_MRT_SetInterval(Chip_MRT_GetRegPtr(2), interval_val);
				break;
				
			case NOT_IN_SET_MODE:
				set_mode = PRE_SET_MODE;
				break;
		}
	}
}

Chip_PININT_BITSLICE_CFG_T Chip_PININT_GetBitsliceConfig(LPC_PIN_INT_T *pPININT, Chip_PININT_BITSLICE_T slice)
{
	uint32_t pmcfg_reg;
		 
	/* Configure bit slice configuration */
	pmcfg_reg = pPININT->PMCFG & (PININT_SRC_BITCFG_MASK << (PININT_SRC_BITCFG_START + (slice * 3)));
	pmcfg_reg = pmcfg_reg >> (PININT_SRC_BITCFG_START + (slice * 3));
	
	return (Chip_PININT_BITSLICE_CFG_T)pmcfg_reg;
}

/* Clock setting - decrement (-)*/
void PININT0_IRQHandler(void)
{
	set_value = -1;
	
	if (Chip_PININT_GetBitsliceConfig(LPC_PININT, PININTBITSLICE0) == PININT_PATTERNLOW)
	{
		Chip_PININT_SetPatternMatchConfig(LPC_PININT, PININTBITSLICE0, PININT_PATTERNHIGH, TRUE);
		if (my_time.curr_displayed == TIME) 
		{
			my_time.seconds = 0;
			time_inc_dec(&my_time, set_value, MINUTES | TIME_ONLY);
		}
		else
		{
			time_inc_dec(&my_time, set_value, DAYS);
		}

		set_mode = SET_MODE_INC;
		blink = FALSE;
		leave_set_mode = 0;
		setupMRT(2, MRT_MODE_REPEAT, INC_START_RATE);/* start at x Hz */
		
	}
	else if (Chip_PININT_GetBitsliceConfig(LPC_PININT, PININTBITSLICE0) == PININT_PATTERNHIGH)
	{
		Chip_PININT_SetPatternMatchConfig(LPC_PININT, PININTBITSLICE0, PININT_PATTERNLOW, TRUE);
		
		set_mode = SET_MODE_BLINK;
		Chip_MRT_SetInterval(Chip_MRT_GetRegPtr(2), 0 | MRT_INTVAL_LOAD); /* stop the timer */
		Chip_MRT_SetDisabled(Chip_MRT_GetRegPtr(2));
	}
}

/* Clock setting - increment (+) */
void PININT1_IRQHandler(void)
{
	set_value = +1;
	
	if (Chip_PININT_GetBitsliceConfig(LPC_PININT, PININTBITSLICE1) == PININT_PATTERNLOW)
	{
		Chip_PININT_SetPatternMatchConfig(LPC_PININT, PININTBITSLICE1, PININT_PATTERNHIGH, TRUE);
		if (my_time.curr_displayed == TIME)
		{
			my_time.seconds = 0;
			time_inc_dec(&my_time, set_value, MINUTES | TIME_ONLY);
		}
		else
		{
			time_inc_dec(&my_time, set_value, DAYS);
		}

		set_mode = SET_MODE_INC;
		blink = FALSE;
		leave_set_mode = 0;
		setupMRT(2, MRT_MODE_REPEAT, INC_START_RATE);/* start at x Hz */

	}
	else if (Chip_PININT_GetBitsliceConfig(LPC_PININT, PININTBITSLICE1) == PININT_PATTERNHIGH)
	{
		Chip_PININT_SetPatternMatchConfig(LPC_PININT, PININTBITSLICE1, PININT_PATTERNLOW, TRUE);
		
		set_mode = SET_MODE_BLINK;
		Chip_MRT_SetInterval(Chip_MRT_GetRegPtr(2), 0 | MRT_INTVAL_LOAD); /* stop the timer */
		Chip_MRT_SetDisabled(Chip_MRT_GetRegPtr(2));
	}
}


void PININT3_IRQHandler(void)
{
	if (LPC_PININT->PMCTRL & PININT_PMCTRL_PMATCH_SEL)
	{
			//my_time.seconds = 33;
		//Chip_PININT_DisablePatternMatch(LPC_PININT);
	}
	
	if (Chip_PININT_GetBitsliceConfig(LPC_PININT, PININTBITSLICE2) == PININT_PATTERNLOW)
	{
		Chip_PININT_SetPatternMatchConfig(LPC_PININT, PININTBITSLICE2, PININT_PATTERNHIGH, FALSE);
		Chip_PININT_SetPatternMatchConfig(LPC_PININT, PININTBITSLICE3, PININT_PATTERNHIGH, TRUE);
		
		setupMRT(2, MRT_MODE_ONESHOT, 1);
	}
	else if (Chip_PININT_GetBitsliceConfig(LPC_PININT, PININTBITSLICE2) == PININT_PATTERNHIGH)
	{		
		Chip_PININT_SetPatternMatchConfig(LPC_PININT, PININTBITSLICE2, PININT_PATTERNLOW, FALSE);
		Chip_PININT_SetPatternMatchConfig(LPC_PININT, PININTBITSLICE3, PININT_PATTERNLOW, TRUE);
		
		/* stop the timer */
		Chip_MRT_SetInterval(Chip_MRT_GetRegPtr(2), 0 | MRT_INTVAL_LOAD); 
		Chip_MRT_SetDisabled(Chip_MRT_GetRegPtr(2));
		
		if (set_mode == PRE_SET_MODE)
		{
			/* enable set mode only after two buttons go high */
			set_mode = SET_MODE_BLINK;
			
			/* Clear pending IRQs for buttons */
			NVIC_ClearPendingIRQ(PININT0_IRQn);
			NVIC_ClearPendingIRQ(PININT1_IRQn);
			/* enable interrupt for +/- buttons handlers */	
			NVIC_EnableIRQ(PININT0_IRQn);
			NVIC_EnableIRQ(PININT1_IRQn);
			
			/* disable interrupt for change displayed data (time/date) */
			NVIC_DisableIRQ(PININT2_IRQn);
		}
		
		if (set_mode == NOT_IN_SET_MODE)
		{			
			if (my_time.curr_displayed == TIME) /* LOCK equals 0 => unlocked */
			{
				my_time.curr_displayed = DATE | LOCK;
			}
			else if (my_time.curr_displayed == DATE)
			{
				my_time.curr_displayed = TIME | LOCK;
			}
			my_time.change_display_timeout = 0;
		}
	}
}

int main(void)
{
	uint8_t mrtch = 0;
	
	/* Initialize system clock */
	SystemInit();
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SWM);
	
	/* Initialize bord, I/O port setting, systick, etc. */
	board_init();
	
	UART_init();
	
	/*------------*/
	//Chip_PININT_Init (LPC_PIN_INT_T *pPININT)
	/* Configure interrupt channel 0 for the GPIO pin in SysCon block */
	Chip_SYSCTL_SetPinInterrupt(0, SW1);
		
	/* Configure channel 0 as wake up interrupt in SysCon block */
	/*Chip_SYSCTL_EnablePINTWakeup(0);*/
	
	/* Configure channel 0 interrupt as edge sensitive and raising/falling edge interrupt */
/*	Chip_PININT_SetPinModeEdge(LPC_PININT, PININTCH0);
	Chip_PININT_EnableIntLow(LPC_PININT, PININTCH0);
	Chip_PININT_EnableIntHigh(LPC_PININT, PININTCH0);*/
	
	/* Configure interrupt channel 1 for the GPIO pin in SysCon block */
	Chip_SYSCTL_SetPinInterrupt(1, SW2);
		
	/* Configure channel 0 as wake up interrupt in SysCon block */
	/*Chip_SYSCTL_EnablePINTWakeup(0);*/
	
	/* Configure channel 1 interrupt as edge sensitive and raising/falling edge interrupt */
	/*Chip_PININT_SetPinModeEdge(LPC_PININT, PININTCH1);
	Chip_PININT_EnableIntLow(LPC_PININT, PININTCH1);
	Chip_PININT_EnableIntHigh(LPC_PININT, PININTCH1);*/
	
	/*------------*/
	
	Chip_PININT_SetPatternMatchSrc(LPC_PININT, 0, PININTBITSLICE0);
	Chip_PININT_SetPatternMatchSrc(LPC_PININT, 1, PININTBITSLICE1);
	Chip_PININT_SetPatternMatchSrc(LPC_PININT, 0, PININTBITSLICE2);
	Chip_PININT_SetPatternMatchSrc(LPC_PININT, 1, PININTBITSLICE3);
	
	Chip_PININT_SetPatternMatchConfig(LPC_PININT, PININTBITSLICE0, PININT_PATTERNLOW, TRUE); 
	Chip_PININT_SetPatternMatchConfig(LPC_PININT, PININTBITSLICE1, PININT_PATTERNLOW, TRUE);	
	Chip_PININT_SetPatternMatchConfig(LPC_PININT, PININTBITSLICE2, PININT_PATTERNLOW, FALSE);
	Chip_PININT_SetPatternMatchConfig(LPC_PININT, PININTBITSLICE3, PININT_PATTERNLOW, TRUE);
	
	Chip_PININT_SetPatternMatchConfig(LPC_PININT, PININTBITSLICE4, PININT_PATTERCONST0, FALSE);
	Chip_PININT_SetPatternMatchConfig(LPC_PININT, PININTBITSLICE5, PININT_PATTERCONST0, FALSE);
	Chip_PININT_SetPatternMatchConfig(LPC_PININT, PININTBITSLICE6, PININT_PATTERCONST0, FALSE);
	
	Chip_PININT_EnablePatternMatch(LPC_PININT);
	
	NVIC_EnableIRQ(PININT3_IRQn);
	
	/* MRT Initialization and disable all timers */
	Chip_MRT_Init();
	for (mrtch = 0; mrtch < MRT_CHANNELS_NUM; mrtch++) {
		Chip_MRT_SetDisabled(Chip_MRT_GetRegPtr(mrtch));
	}
	
	/* Enable the interrupt for the MRT */
	NVIC_EnableIRQ(MRT_IRQn);

	/* Enable timer 0 in repeat mode - main period for multiplexing*/
	setupMRT(0, MRT_MODE_REPEAT, REFRESH_RATE);
	
	/* Enable timer 3 in repeat mode - */
	setupMRT(3, MRT_MODE_REPEAT, ROLL_RATE);

	
	/* Playground starts here */
	my_time.seconds = 0;
	my_time.minutes = 1;
	my_time.hours = 12;
	
	my_time.days = 31;
	my_time.months = 12;
	my_time.years = 2017;
	
	my_time.curr_displayed = TIME;
	my_time.change_display_timeout = 0;
	
	while(my_time.seconds == 0); /* wait one second prior setting BT to give it enough time to startup */
	while (!set_BT_power_save (&my_time));
	
	while(1)
	{		
		UART_commands_exec(&my_time);	
		//buttonishi=!Chip_GPIO_ReadPortBit(LPC_GPIO_PORT, 0, SW1);              //button is hi
		//buttonishioneshot=buttonishi && !buttonishil; //button just went hi
	}
}
