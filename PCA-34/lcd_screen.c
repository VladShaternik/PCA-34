/*
 * lcd_screen.c
 *
 * Created: 5/8/2014 3:09:24 PM
 *  Author: ENG3
 */ 

#include "lcd_screen.h"

void initialize_Screen()
{
	_delay_ms(50);
	unsigned char message[10];
	
	message[0] = (0x00);							//Control bit with continuous data stream
	message[1] = (0x38);							//Function Set (8-bit data, 1 Line 5x8 character mode, Normal Instruction Table Chosen)
	TWI_MasterWrite(&twiMaster, LCD_SLAVE_ADDRESS, message, 2);
	while(twiMaster.status == 1);
	_delay_ms(10);
	if (twiMaster.result == TWIM_RESULT_NACK_RECEIVED)
	{
		initialize_Screen();
		return;
	}
	
	message[0] = (0x00);
	message[1] = (0x39);							//Function Set (8-bit data, 1 Line 5x8 character mode, Extension Instruction Table Chosen)
	TWI_MasterWrite(&twiMaster, LCD_SLAVE_ADDRESS, message, 2);
	while(twiMaster.status == 1);
	_delay_ms(10);
	if (twiMaster.result == TWIM_RESULT_NACK_RECEIVED)
	{
		initialize_Screen();
		return;
	}
	
	message[0] = (0x00);
	message[1] = (0x14);							//Set frame frequency to 192 Hz and Voltage Bias to 1/5
	message[2] = (0x78);							//Set contrast bits C3:0 to 8 (C5:0 - 0x28 *C5:4 is part of next data byte)
	message[3] = (0x5E);							//Turn on Icon Display and Booster Circuit and set C5:4 to 2 for contrast setting
	message[4] = (0x6D);							//Turn on internal follower circuit and adjust V0 generator amplified ratio (Rab2:0 - 2)
	message[5] = (0x0C);
	message[6] = (0x01);
	message[7] = (0x06);
	TWI_MasterWrite(&twiMaster, LCD_SLAVE_ADDRESS, message, 8);
	while(twiMaster.status == 1);
	_delay_ms(100);
	if (twiMaster.result == TWIM_RESULT_NACK_RECEIVED)
	{
		initialize_Screen();
		return;
	}
	
	_delay_ms(50);
	load_custom_characters();
}

// Driver DDRAM addressing
const uint8_t dram_dispAddr [][3] =
{
	{ 0x00, 0x00, 0x00 },  // One line display address
	{ 0x00, 0x40, 0x00 },  // Two line display address
	{ 0x00, 0x10, 0x20 }   // Three line display address
};

void setCursor(uint8_t line_num, uint8_t x)
{
	if (line_num == 0)
	{
		lcd_command(SET_DDRAM_ADDRESS | (0b0000000000 & x));
	}
	else if (line_num == 1)
	{
		lcd_command(SET_DDRAM_ADDRESS | (0b0001000000 + x));
	}
}

void lcd_write(char *message)
{
	char size = strlen(message);
	unsigned char complete_message[size + 1];
	
	complete_message[0] = (0x40);			//Control bit with R/S set high and continuous bytes
	
	for(unsigned char i = 0; i < size; i ++)
	{
		complete_message[i + 1] = message[i];
	}

	TWI_MasterWrite(&twiMaster, LCD_SLAVE_ADDRESS, complete_message, size + 1);
	while(twiMaster.status == 1);
	_delay_ms(10);
	
	while (twiMaster.result == TWIM_RESULT_NACK_RECEIVED)
	{
		initialize_Screen();
		TWI_MasterWrite(&twiMaster, LCD_SLAVE_ADDRESS, complete_message, size + 1);
		while(twiMaster.status == 1);
		_delay_ms(10);
	}
}


void display_custom_character(uint8_t car_num)
{
	unsigned char complete_message[2];
	complete_message[0] = (0x40);			//Control bit with R/S set high and continuous bytes
	complete_message[1] = (car_num);
	
	TWI_MasterWrite(&twiMaster, LCD_SLAVE_ADDRESS, complete_message, 2);
	while(twiMaster.status == 1);
	_delay_ms(10);
	
	while (twiMaster.result == TWIM_RESULT_NACK_RECEIVED)
	{
		initialize_Screen();
		TWI_MasterWrite(&twiMaster, LCD_SLAVE_ADDRESS, complete_message, 2);
		while(twiMaster.status == 1);
		_delay_ms(10);
	}
}

/*
After acknowledgement, one or more command words follow which define the status of the addressed slaves.
A command word consists of a control byte, which defines Co and RS, plus a data byte.
The last control byte is tagged with a cleared most significant bit (i.e. the continuation bit Co). After a
control byte with a
cleared Co bit, only data bytes will follow. The state of the RS bit defines whether the data byte is interpreted
as a command
or as RAM data. All addressed slaves on the bus also acknowledge the control and data bytes. After the last
control byte,
depending on the RS bit setting; either a series of display data bytes or command data bytes may follow. If the
RS bit is set
to logic 1, these display bytes are stored in the display RAM at the address specified by the data pointer. The
data pointer is
automatically updated and the data is directed to the intended ST7036i device. If the RS bit of the last control
byte is set to
logic 0, these command bytes will be decoded and the setting of the device will be changed according to the
received
commands. Only the addressed slave makes the acknowledgement after each byte.
*/
void lcd_command(uint16_t command)
{
	unsigned char complete_message[2];
	uint8_t volatile comm;
	comm = (uint8_t)(command >> 2);
	complete_message[0] = comm & 0b11000000;                      // Control byte
	complete_message[1] = (uint8_t)(command);                     // Data byte
	TWI_MasterWrite(&twiMaster, LCD_SLAVE_ADDRESS, complete_message, 2);
	while(twiMaster.status == 1);
	_delay_ms(10);
	
	while (twiMaster.result == TWIM_RESULT_NACK_RECEIVED)
	{
		initialize_Screen();
		TWI_MasterWrite(&twiMaster, LCD_SLAVE_ADDRESS, complete_message, 2);
		while(twiMaster.status == 1);
		_delay_ms(10);
	}
}

void load_custom_characters()
{
	
	unsigned char message[2];
	message[0] = (0x00);							//Control bit with continuous data stream
	message[1] = (0x38);							//Function Set (8-bit data, 1 Line 5x8 character mode, Normal Instruction Table Chosen)
	TWI_MasterWrite(&twiMaster, LCD_SLAVE_ADDRESS, message, 2);
	while(twiMaster.status == 1);
	_delay_ms(10);
	while (twiMaster.result == TWIM_RESULT_NACK_RECEIVED)
	{
		initialize_Screen();
		TWI_MasterWrite(&twiMaster, LCD_SLAVE_ADDRESS, message, 2);
		while(twiMaster.status == 1);
		_delay_ms(10);
	}
	
	// 1 bar
	lcd_command(SET_CGRAM_ADDRESS);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000010000);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000010000);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000010000);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000010000);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000010000);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000010000);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000010000);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000010000);
	//2 bar
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011000);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011000);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011000);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011000);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011000);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011000);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011000);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011000);
	//3 bar
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011100);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011100);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011100);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011100);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011100);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011100);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011100);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011100);
	//4 bar
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011110);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011110);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011110);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011110);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011110);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011110);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011110);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011110);
	//5 bar
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011111);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011111);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011111);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011111);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011111);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011111);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011111);
	lcd_command(WRITE_DATA_TO_RAM | 0b0000011111);
	lcd_command(SET_DDRAM_ADDRESS);
}