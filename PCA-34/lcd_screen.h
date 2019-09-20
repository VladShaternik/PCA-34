/*
 * lcd_screen.h
 *
 * Created: 5/8/2014 4:24:51 PM
 *  Author: ENG3
 */ 


#ifndef LCD_SCREEN_H_
#define LCD_SCREEN_H_

#define LCD_SLAVE_ADDRESS 0x3C

#include "twi_master_driver.h"
#include <util/delay.h>
#include <string.h>
#include <stdio.h>


enum LCD_COMMANDS
{
	CLEAR_DISPLAY           = 0b0000000001,
	RETURN_HOME             = 0b0000000010,
	ENTRY_MODE_SET          = 0b0000000100,
	DISPLAY_ON_OFF          = 0b0000001000,
	CURSOR_OR_DISPLAY_SHIFT = 0b0000010000,
	FUNCTION_SET            = 0b0000100000,
	SET_CGRAM_ADDRESS       = 0b0001000000,
	SET_DDRAM_ADDRESS       = 0b0010000000,
	READ_BUSY_FLAG_AND_ADDR = 0b0000000000,
	WRITE_DATA_TO_RAM       = 0b0100000000,
	READ_DATA_FROM_RAM      = 0b0100000000
};

uint16_t lcd_screen_update[2][20];
uint16_t lcd_screen[2][20];
uint8_t lcd_row_counter;
uint8_t lcd_col_counter;

void initialize_Screen(void);
void lcd_write(char *);
void lcd_command(uint16_t command);
void load_custom_characters();
void display_character(uint8_t car_num);
void lcd_set_cursor(char line, char location);
void lcd_clear_screen(void);
void lcd_backspace(void);
void lcd_write_hex(char message);
void setCursor(uint8_t line_num, uint8_t x);
void clear_lcd_update();

TWI_Master_t twiMaster;    /*!< TWI master module. */

#endif /* LCD_SCREEN_H_ */