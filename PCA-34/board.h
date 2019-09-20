#include <avr/io.h>
#define F_CPU 20000000 // 20MHz clock

#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include "twi_master_driver.h"
#include "lcd_screen.h"
#include "ina226.h"

#define FALSE 0
#define TRUE 1

#define INA_ADDRESS 0x41

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

#define MAX_CURRENT 2500
#define MAX_VOLTAGE 5000

/* Display/Encoder Variables */
volatile uint8_t encoder_val;
volatile uint8_t encoder_val_tmp;
volatile uint16_t desired_current_temp;
volatile uint16_t desired_current;
volatile uint16_t current;
volatile uint16_t temp_current;
volatile uint16_t voltage;
volatile uint16_t temp_voltage;
volatile bool     update;
volatile bool     set_current_mode;
volatile uint8_t  set_current_mode_on;
volatile uint32_t rtc_idle_counter;
volatile uint32_t rtc_counter;
volatile uint32_t rtc_counter_prev;
volatile bool     rtc_counter_1_4;
volatile bool     rtc_counter_1_2;
volatile bool     encoder_sw_was_low;

void initialize();
void display_current_voltage(uint16_t current, uint16_t voltage);
void display_set_current();
uint8_t debounce(volatile unsigned char *port, uint8_t pin, uint8_t precisionUs, uint8_t maxUs);
uint8_t read_gray_code_from_encoder();
void handle_encoder();
void display_danger();
void lcd_update(bool two_line);
