/*
 * Main Controller.c
 *
 * Created: 8/17/2018 1:48:41 PM
 * Author : ENG_3
 */ 

/*
 * RCB-1600 Test Code.c
 *
 * Created: 8/16/2018 11:09:35 AM
 * Author : ENG_3
 */ 

#include <avr/io.h>
#define F_CPU 20000000 // 20MHz clock

#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include "twi_master_driver.h"
#include "lcd_screen.h"

#define FALSE 0
#define TRUE 1

enum PINS 
{
	LTCH                 = 2, 
	SRVC_REQ             = 3, 
	BOTH_EDGES_INTERRUPT = 0, 
	GRN_LED              = 2, 
	RED_LED              = 3, 
	LCD_BCKLGHT          = 7, 
	ENCDR_A              = 6, 
	ENCDR_B              = 5, 
	ENCDR_SW             = 4
};

/* I2C Baudrate Settings (CPU speed 20MHz, BAUDRATE 100kHz and Baudrate Register Settings) */
#define CPU_SPEED 20000000
#define TWI_BAUDRATE 100000
#define TWI_BAUDSETTING TWI_BAUD(CPU_SPEED, TWI_BAUDRATE)

/* Display/Encoder Variables */
uint8_t a_low = FALSE;
uint8_t b_low = FALSE;
uint8_t both_low = FALSE;
uint8_t holding_click = FALSE;

#define MAX_CURRENT 2500

void initialize(){
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
	PORTA_PIN5CTRL = (1 << PORT_PULLUPEN_bp); //|| (1 << BOTH_EDGES_INTERRUPT);
	PORTA_PIN6CTRL = (1 << PORT_PULLUPEN_bp); //|| (1 << BOTH_EDGES_INTERRUPT);
	PORTA_PIN4CTRL |= (0x3); //interrupt on falling edge
	PORTA_PIN5CTRL |= (0x1); //interrupt on both edges
	PORTA_PIN6CTRL |= (0x1); //interrupt on both edges
	
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
}

void display_current_voltage(uint16_t current, uint16_t voltage)
{
	lcd_command(CLEAR_DISPLAY);
	
	int length = snprintf(NULL, 0, "%02d.%dA", current / 100, current / 10 % 10);
	char* str = malloc(length + 1);
	snprintf(str, length + 1, "%02d.%dA", current / 100, current / 10 % 10);
	lcd_write(str);
	free(str);
	
	current = (current + 50) / 100; // round to nearest hundred and get hundreds
	
	uint8_t full_bars;
	uint8_t last_bar;
	
	full_bars = current / 5;
	last_bar  = current % 5;
	
	for(int i = 0; i < full_bars; i++)
	{
		display_custom_character(4);
	}
	
	if (last_bar != 0)
	{
		display_custom_character(last_bar - 1);
	}
	
	setCursor(1, 0);
	
	length = snprintf(NULL, 0, "%02d.%dV", voltage / 100, voltage / 10 % 10);
	str = malloc(length + 1);
	snprintf(str, length + 1, "%02d.%dV", voltage / 100, voltage / 10 % 10);
	lcd_write(str);
	free(str);
	
	voltage = (voltage + 50) / 100; // round to nearest hundred and get hundreds
	
	full_bars = voltage / 5;
	last_bar  = voltage % 5;
	
	for(int i = 0; i < full_bars; i++)
	{
		display_custom_character(4);
	}
	
	if (last_bar != 0)
	{
		display_custom_character(last_bar - 1);
	}
}

volatile uint16_t current;
volatile uint16_t voltage;
volatile bool     update;

int main()
{
	//Pin/Settings initialization
    initialize();
    sei();
	
	//I2C Setup
	TWI_MasterInit(&twiMaster, &TWI0, TWI_BAUDSETTING);
	//LCD screen initialization
	_delay_ms(500);
	initialize_Screen();
	_delay_ms(500);
	
	current = 0;
	voltage = 0;
	update  = TRUE;
	
    while (1)
    {
		if (update)
		{
			voltage = current * (current / 500);
			display_current_voltage(current, voltage);
			update = FALSE;
		}
    }
}

ISR(TWI0_TWIM_vect)
{
	TWI_MasterInterruptHandler(&twiMaster);
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

ISR(PORTA_PORT_vect)//interrupts from an encoder turn or push
{
	bool is_debounce_successful = TRUE;
	is_debounce_successful &= debounce(&PORTA_IN, ENCDR_SW, 10, 60);
	is_debounce_successful &= debounce(&PORTA_IN, ENCDR_A, 10, 60);
	is_debounce_successful &= debounce(&PORTA_IN, ENCDR_B, 10, 60);
		
	if (is_debounce_successful)
	{
		if(!(PORTA_IN & (1 << ENCDR_SW)))//if switch pin is low
		{
				
		}
		else if(both_low)
		{
			if((PORTA_IN & (1 << ENCDR_B)))//left turn
			{
				if (current >= 10)
				{
					current -= 10;
					update = TRUE;
				}
			}
			else if((PORTA_IN & (1 << ENCDR_A))) //right turn
			{
				if (current <= MAX_CURRENT - 10)
				{
					current += 10;
					update = TRUE;
				}
			}
			both_low = FALSE;
			b_low = FALSE;
			a_low = FALSE;
			_delay_ms(2);
		}
		else if(!(PORTA_IN & (1 << ENCDR_B)) && !b_low)
		{
			b_low = TRUE;
			if(a_low)
			{
				both_low = TRUE;
				_delay_ms(1);
				b_low = FALSE;
				a_low = FALSE;
			}
		}
		else if(!(PORTA_IN & (1 << ENCDR_A)) && !a_low)
		{
			a_low = TRUE;
			if(b_low)
			{
				both_low = TRUE;
				_delay_ms(1);
				b_low = FALSE;
				a_low = FALSE;
			}
		}
		else if(b_low && PORTA_IN & (1 << ENCDR_A))//if B is low and A is high
		{
			a_low = FALSE;
			b_low = FALSE;
			both_low = FALSE;
		}
		else if(a_low && PORTA_IN & (1 << ENCDR_B)) //if A is low and B is high
		{
			a_low = FALSE;
			b_low = FALSE;
			both_low = FALSE;
		}
	}
	
	PORTA_INTFLAGS = 0xFF; //clear port interrupt flags
}

