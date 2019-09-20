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
	
	current              = 0;
	temp_current         = 0;
	voltage              = 0;
	temp_voltage         = 0;
	update               = TRUE;
	set_current_mode     = FALSE;
	set_current_mode_on  = 0;
	rtc_idle_counter     = 0;
	rtc_counter          = 0;
	rtc_counter_1_4      = FALSE;
	rtc_counter_1_2      = FALSE;
	rtc_counter_prev     = 0;
	encoder_sw_was_low   = FALSE;
	
	desired_current = eeprom_read_word((uint16_t*) 0x00);
	
	if (desired_current < 0 || desired_current > MAX_CURRENT)
	{
		desired_current = 0;
		eeprom_update_word((uint16_t*) 0x00, desired_current);
	}
	
	desired_current_temp = desired_current;
}

void display_current_voltage(uint16_t cur, uint16_t vol)
{
	lcd_col_counter = 0;
	lcd_row_counter = 0;
	
	
	// START writing voltage number in format %02d.%dV
	
	if (vol > 7000)
	{
		vol = 7000;
	}
	
	int length = snprintf(NULL, 0, "%02d.%dV ", vol / 100, vol / 10 % 10);
	char* str = malloc(length + 1);
	snprintf(str, length + 1, "%02d.%dV ", vol / 100, vol / 10 % 10);
	
	for(unsigned char i = 0; i < length; i++)
	{
		lcd_screen_update[lcd_row_counter][lcd_col_counter] = str[i];
		lcd_col_counter++;
	}
	
	free(str);
	
	vol = (vol + 50) / 100; // round to nearest hundred and get hundreds
	
	uint8_t full_bars = vol / 5;
	uint8_t last_bar  = vol % 5;
	
	for(int i = 0; i < full_bars; i++)
	{
		if (i >= ((MAX_VOLTAGE + 50) / 100) / 5 - 1 && vol >= (MAX_VOLTAGE + 50) / 100)
		{
			lcd_screen_update[lcd_row_counter][lcd_col_counter] = 6;
			lcd_col_counter++;
			if (last_bar != 0)
			{
				lcd_screen_update[lcd_row_counter][lcd_col_counter] = 6;
				lcd_col_counter++;
				last_bar = 0;
			}
		}
		else
		{
			if ((i + 1) % 2 == 0)
			{
				lcd_screen_update[lcd_row_counter][lcd_col_counter] = 5;
				lcd_col_counter++;
			}
			else
			{
				lcd_screen_update[lcd_row_counter][lcd_col_counter] = 4;
				lcd_col_counter++;
			}
		}
	}
	
	if (last_bar != 0)
	{
		lcd_screen_update[lcd_row_counter][lcd_col_counter] = last_bar - 1;
		lcd_col_counter++;
	}
	
	for (int i = full_bars; i < 20; i++)
	{
		lcd_screen_update[lcd_row_counter][lcd_col_counter] = ' ';
		lcd_col_counter++;
	}
	// FINISH writing voltage number
	
	lcd_row_counter++;
	lcd_col_counter = 0;
	
	// START writing current number in format %02d.%dA
	
	length = snprintf(NULL, 0, "%02d.%dA ", cur / 100, cur / 10 % 10);
	str = malloc(length + 1);
	snprintf(str, length + 1, "%02d.%dA ", cur / 100, cur / 10 % 10);
	
	for(unsigned char i = 0; i < length; i++)
	{
		lcd_screen_update[lcd_row_counter][lcd_col_counter] = str[i];
		lcd_col_counter++;
	}
	
	free(str);
	// FINISH writing current number
	
	// START writing current bars
	cur = (cur + 5) / 10; // round to nearest tenth and get tenth
	
	full_bars = cur / 25;
	last_bar  = (cur % 25) / 5;
	
	for(int i = 0; i < full_bars; i++)
	{
		if (i == full_bars - 1 && cur == (MAX_CURRENT + 5) / 10)
		{
			lcd_screen_update[lcd_row_counter][lcd_col_counter] = 6;
			lcd_col_counter++;
		}
		else
		{
			if ((i + 1) % 4 == 0)
			{
				lcd_screen_update[lcd_row_counter][lcd_col_counter] = 5;
				lcd_col_counter++;
			}
			else
			{
				lcd_screen_update[lcd_row_counter][lcd_col_counter] = 4;
				lcd_col_counter++;
			}
		}
	}
	
	if (last_bar != 0)
	{
		lcd_screen_update[lcd_row_counter][lcd_col_counter] = last_bar - 1;
		lcd_col_counter++;
	}
	
	for (int i = full_bars; i < 20; i++)
	{
		lcd_screen_update[lcd_row_counter][lcd_col_counter] = ' ';
		lcd_col_counter++;
	}
	// FINISH writing current bars
	
	lcd_update(TRUE);
}

void lcd_update(bool two_line)
{
	unsigned char n = two_line ? 2 : 1;
	for (unsigned char i = 0; i < n; i++)
	{
		for (unsigned char j = 0; j < 20; j++)
		{
			if (lcd_screen[i][j] != lcd_screen_update[i][j])
			{
				setCursor(i, j);
				display_character(lcd_screen_update[i][j]);
				lcd_screen[i][j] = lcd_screen_update[i][j];
			}
		}
	}
}

void display_set_current()
{
	// START writing current number in format %02d.%dA
	int length = snprintf(NULL, 0, "SET CUR");
	char* str = malloc(length + 1);
	snprintf(str, length + 1, "SET CUR");
	
	for(unsigned char i = 0; i < length; i++)
	{
		lcd_screen_update[lcd_row_counter][lcd_col_counter] = str[i];
		lcd_col_counter++;
	}
	
	free(str);
	
	length = snprintf(NULL, 0, "RENT: ");
	str = malloc(length + 1);
	snprintf(str, length + 1, "RENT: ");
	
	for(unsigned char i = 0; i < length; i++)
	{
		lcd_screen_update[lcd_row_counter][lcd_col_counter] = str[i];
		lcd_col_counter++;
	}
	
	free(str);
	
	length = snprintf(NULL, 0, "%02d.%dA", desired_current / 100, desired_current / 10 % 10);
	str = malloc(length + 1);
	snprintf(str, length + 1, "%02d.%dA", desired_current / 100, desired_current / 10 % 10);
	
	for(unsigned char i = 0; i < length; i++)
	{
		lcd_screen_update[lcd_row_counter][lcd_col_counter] = str[i];
		lcd_col_counter++;
	}
	
	free(str);
	// FINISH writing current number
	
	lcd_col_counter = 0;
	lcd_row_counter = 0;
	
	lcd_update(FALSE);
}

void display_danger()
{
	lcd_command(CLEAR_DISPLAY);
	lcd_command(FUNCTION_SET | 0b0000111100);
	lcd_write(" OVER ");
	lcd_write("VOLTAGE");
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
	
	desired_current = (desired_current / 10) * 10;

	if(encoder_val != encoder_val_tmp)
	{
		if((encoder_val == 0 && encoder_val_tmp == 2))
		{
			if (desired_current <= MAX_CURRENT - 10)
			{
				if (rtc_counter - rtc_counter_prev < 2)
				{
					if (desired_current <= MAX_CURRENT - 100)
					{
						desired_current += 100;
						update = TRUE;
					}
					else
					{
						desired_current += 10;
						update = TRUE;
					}
				}
				else
				{
					desired_current += 10;
					update = TRUE;
				}
			}
		}
		else if((encoder_val == 1 && encoder_val_tmp == 3))
		{
			if (desired_current >= 10)
			{
				if (rtc_counter - rtc_counter_prev < 2)
				{
					if (desired_current >= 100)
					{
						desired_current -= 100;
						update = TRUE;
					}
					else
					{
						desired_current -= 10;
						update = TRUE;
					}
				}
				else
				{
					desired_current -= 10;
					update = TRUE;
				}
			}
		}

		encoder_val = encoder_val_tmp;
		rtc_counter_prev = rtc_counter;
	}
	
}