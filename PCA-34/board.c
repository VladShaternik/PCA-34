#include "board.h"

void initialize()
{
	CPU_CCP = 0xD8; //temporarily remove protection for protected I/O registers (Cannot change clock prescaler without doing this first)
	CLKCTRL_MCLKCTRLB = (0 << 0); // disable prescaler (There's a default prescaler of 6)
	_delay_ms(5);
	CLKCTRL_MCLKCTRLA |= (1 << 7); // System clock is output to CLKOUT pin (20 MHz)
	
	/* *** Port A Pins Setup ***
	PA0 - RST/UPDI
	PA1 - Buzzer - Output/Low
	PA2 - LED GREEN - Output/Low
	PA3 - LED RED - Output/Low
	PA4 - SW_Encoder - Input/Pull-up
	PA5 - B_Encoder - Input/Pull-up - Interrupt both edges
	PA6 - A_Encoder - Input/Pull-up - Interrupt both edges
	PA7 - Display LED (Backlight) - Output/Low
	*/
	PORTA_DIR = 0b10001111;
	PORTA_PIN4CTRL = (1 << PORT_PULLUPEN_bp);
	PORTA_PIN5CTRL = (1 << PORT_PULLUPEN_bp);
	PORTA_PIN6CTRL = (1 << PORT_PULLUPEN_bp);
	
	/* *** Port B Pins Setup ***
	PB0 - SCL out - Output/High
	PB1 - SDA - Input/Pull-up
	PB2 - LTCH (Falling edge trig/LCD) - Output/High
	PB3 - Service Request - Input/Pull-up
	PB4 - D4_LCD - Output/Low
	PB5 - D5_LCD - Output/Low
	PB6 - D6_LCD - Output/Low
	PB7 - D7_LCD - Output/Low
	*/
	PORTB_DIR = 0b11110101;
	PORTB_OUTSET = (1 << PIN0_bp);
	PORTB_PIN1CTRL = (1 << PORT_PULLUPEN_bp);
	//PORTB_PIN3CTRL = (1 << PORT_PULLUPEN_bp);
	PORTB_PIN3CTRL |= (0x1); //interrupt on both edges
	CPUINT.LVL1VEC = 0x08; //Set PORTB interrupt to be LEVEL1
	
	/* *** Port C Pins Setup ***
	PC0 - D0_LCD - Output/Low
	PC1 - D1_LCD - Output/Low
	PC2 - D2_LCD - Output/Low
	PC3 - D3_LCD - Output/Low
	PC4 - R/W (0:RD, 1:WR) - Output/Low
	PC5 - RS (Reg Sel) (0:CMD, 1:DATA) - Output/Low
	*/
	PORTC_DIR = 0b00111111;
	
	//RTC initialization
	while (RTC.STATUS > 0) {} // Wait for all register to be synchronized

	RTC.PER = 1;
	RTC.INTCTRL = 0 << RTC_CMP_bp
	| 1 << RTC_OVF_bp; //Overflow interrupt.
	
	RTC.CTRLA = RTC_PRESCALER_DIV1_gc	//NO Prescaler
	| 1 << RTC_RTCEN_bp       	//Enable RTC
	| 1 << RTC_RUNSTDBY_bp;   	//Run in standby

	RTC.CLKSEL = RTC_CLKSEL_INT1K_gc; // 32KHz divided by 32, i.e run at 1.024kHz
	
	encoder_val = read_gray_code_from_encoder();
	encoder_val_tmp = 0;
	
	desired_current     = 0;
	current             = 0;
	temp_current        = 0;
	voltage             = 0;
	temp_voltage        = 0;
	update              = TRUE;
	set_current_mode    = FALSE;
	rtc_idle_counter    = 0;
	rtc_counter         = 0;
	rtc_counter_1_4     = FALSE;
	rtc_counter_1_2     = FALSE;
	rtc_counter_prev    = 0;
	set_current_blink   = FALSE;
}


void display_current_voltage(uint16_t current, uint16_t voltage)
{
	lcd_command(CLEAR_DISPLAY);
	lcd_command(FUNCTION_SET | 0b0000111000);
	
	int length = snprintf(NULL, 0, "%02d.%dA ", current / 100, current / 10 % 10);
	char* str = malloc(length + 1);
	snprintf(str, length + 1, "%02d.%dA ", current / 100, current / 10 % 10);
	lcd_write(str);
	free(str);
	
	current = (current + 5) / 10; // round to nearest tenth and get tenth
	
	uint8_t full_bars;
	uint8_t last_bar;
	
	full_bars = current / 25;
	last_bar  = (current % 25) / 5;
	
	for(int i = 0; i < full_bars; i++)
	{
		if (i == full_bars - 1 && current == (MAX_CURRENT + 5) / 10)
		{
			display_custom_character(6);
		}
		else
		{
			if ((i + 1) % 4 == 0)
			{
				display_custom_character(5);
			}
			else
			{
				display_custom_character(4);
			}
		}
	}
	
	if (last_bar != 0)
	{
		display_custom_character(last_bar - 1);
	}
	
	setCursor(1, 0);
	
	if (voltage > 7000)
	{
		voltage = 7000;
	}
	
	length = snprintf(NULL, 0, "%02d.%dV ", voltage / 100, voltage / 10 % 10);
	str = malloc(length + 1);
	snprintf(str, length + 1, "%02d.%dV ", voltage / 100, voltage / 10 % 10);
	lcd_write(str);
	free(str);
	
	voltage = (voltage + 50) / 100; // round to nearest hundred and get hundreds
	
	full_bars = voltage / 5;
	last_bar  = voltage % 5;
	
	for(int i = 0; i < full_bars; i++)
	{
		if (i >= ((MAX_VOLTAGE + 50) / 100) / 5 - 1 && voltage >= (MAX_VOLTAGE + 50) / 100)
		{
			display_custom_character(6);
			if (last_bar != 0)
			{
				display_custom_character(6);
				last_bar = 0;
			}
		}
		else
		{
			if ((i + 1) % 2 == 0)
			{
				display_custom_character(5);
			}
			else
			{
				display_custom_character(4);
			}
		}
	}
	
	if (last_bar != 0)
	{
		display_custom_character(last_bar - 1);
	}
}

void display_voltage(uint16_t voltage)
{
	lcd_command(CLEAR_DISPLAY);
	lcd_command(FUNCTION_SET | 0b0000111000);
	
	uint8_t full_bars;
	uint8_t last_bar;
	
	setCursor(1, 0);
	
	if (voltage > 7000)
	{
		voltage = 7000;
	}
	
	int length = snprintf(NULL, 0, "%02d.%dV ", voltage / 100, voltage / 10 % 10);
	char* str = malloc(length + 1);
	snprintf(str, length + 1, "%02d.%dV ", voltage / 100, voltage / 10 % 10);
	lcd_write(str);
	free(str);
	
	voltage = (voltage + 50) / 100; // round to nearest hundred and get hundreds
	
	full_bars = voltage / 5;
	last_bar  = voltage % 5;
	
	for(int i = 0; i < full_bars; i++)
	{
		if (i >= ((MAX_VOLTAGE + 50) / 100) / 5 - 1 && voltage >= (MAX_VOLTAGE + 50) / 100)
		{
			display_custom_character(6);
			if (last_bar != 0)
			{
				display_custom_character(6);
				last_bar = 0;
			}
		}
		else
		{
			if ((i + 1) % 2 == 0)
			{
				display_custom_character(5);
			}
			else
			{
				display_custom_character(4);
			}
		}
	}
	
	if (last_bar != 0)
	{
		display_custom_character(last_bar - 1);
	}
}

void display_danger()
{
	lcd_command(CLEAR_DISPLAY);
	lcd_command(FUNCTION_SET | 0b0000111100);
	lcd_write("  D A ");
	lcd_write("N G E R");
	lcd_write(" ! ! !");
}

/************************************************************************/
/* FUNCTION - debounce                                                  */
/* This function debounces switches, encoders, buttons...               */
/* It counts amount of highs and lows until one of them hits            */
/* @precision times in a row                                            */
/*                                                                      */
/************************WARNING!!!**************************************/
/*         USE CAREFULL THIS FUNCTION INSIDE INTERRAPT!                 */
/*               (IT COULD TAKE TOO MUCH TIME)                          */
/************************WARNING!!!**************************************/
/*                                                                      */
/* ASSUMPTIONS: This function assumes that you have TRUE and FALSE      */
/*              declared as macros (#define)                            */
/*                                                                      */
/* @port - address of port where located pin that needs to be debounced */
/*         (example - &PORTA_IN)                                        */
/* @pin - pin in the port that needs to be debounced                    */
/*        (example - 3)                                                 */
/* @precisionUs - how many times in a row should highs or lows hit. It  */
/*                is also equals to the time in ms of the delay that    */
/*                this function causes                                  */
/*                (example - 20)                                        */
/* @maxUs - max amt of microseconds to last for this function (usually  */
/*          6 times more than @precisionUs)                             */
/* @return - whether the function debounced pin correctly (If running   */
/*           time of the function reached @maxUs returns false(0))      */
/************************************************************************/
uint8_t debounce(volatile unsigned char *port, uint8_t pin, uint8_t precisionUs, uint8_t maxUs)
{
	uint8_t counter = 0;
	uint8_t counterUs = 0;
	uint8_t isHigh = FALSE;
	
	while(counter != precisionUs && counterUs != maxUs)
	{
		// *(volatile uint8_t *)(port) - is a dereference of a port's address, therefore, we read directly from memory
		if(*(volatile uint8_t *)(port) & (1 << pin))
		{
			counter = isHigh ? counter + 1 : 0;
			isHigh = TRUE;
		}
		else
		{
			counter = isHigh ? 0 : counter + 1;
			isHigh = FALSE;
		}
		_delay_us(1);
		counterUs++;
	}
	
	return counterUs != maxUs;
}

uint8_t read_gray_code_from_encoder()
{
	uint8_t val=0;
	
	bool is_debounce_successful = TRUE;
	is_debounce_successful &= debounce(&PORTA_IN, ENCDR_A, 10, 60);
	is_debounce_successful &= debounce(&PORTA_IN, ENCDR_B, 10, 60);
	
	if (is_debounce_successful)
	{
		if(!(PORTA_IN & (1 << ENCDR_A)))
		{
			val |= (1 << 1);
		}

		if(!(PORTA_IN & (1 << ENCDR_B)))
		{
			val |= (1<<0);
		}
	}

	return val;
}

void handle_encoder()
{
	encoder_val_tmp = read_gray_code_from_encoder();
	
	current = (current / 10) * 10;

	if(encoder_val != encoder_val_tmp)
	{
		if((encoder_val == 0 && encoder_val_tmp == 2))
		{
			if (current <= MAX_CURRENT - 10)
			{
				if (rtc_counter - rtc_counter_prev < 2)
				{
					if (current <= MAX_CURRENT - 100)
					{
						current += 100;
						update = TRUE;
					}
					else
					{
						current += 10;
						update = TRUE;
					}
				}
				else
				{
					current += 10;
					update = TRUE;
				}
			}
		}
		else if((encoder_val == 1 && encoder_val_tmp == 3))
		{
			if (current >= 10)
			{
				if (rtc_counter - rtc_counter_prev < 2)
				{
					if (current >= 100)
					{
						current -= 100;
						update = TRUE;
					}
					else
					{
						current -= 10;
						update = TRUE;
					}
				}
				else
				{
					current -= 10;
					update = TRUE;
				}
			}
		}

		encoder_val = encoder_val_tmp;
		rtc_counter_prev = rtc_counter;
	}
	
}