/*
 * ina226.c
 *
 * Created: 4/3/2018 3:17:11 PM
 *  Author: ENG_3
 */ 
#include "twi_master_driver.h"
#include "main.h"

void INA_Write(TWI_Master_t *twi, uint8_t register_addr, uint8_t data_byte_1, uint8_t data_byte_2, uint8_t ina226_addr)
{
	char message[8];
	message[0] = register_addr;
	message[1] = data_byte_1; // MSB
	message[2] = data_byte_2; // LSB
	TWI_MasterWrite(twi, ina226_addr, (uint8_t *) message, 3);
	
	_delay_ms(20);
}

register8_t * INA_Read_Short(TWI_Master_t *twi, uint8_t register_addr, uint8_t ina226_addr)
{
	char message[8];
	message[0] = register_addr;
	TWI_MasterWriteRead(twi, ina226_addr, (uint8_t *) message, 1, 2);
	
	_delay_ms(20);
	
	return twi->readData;
}

register8_t * INA_Read_Long(TWI_Master_t *twi, uint8_t register_addr, uint8_t ina226_addr)
{
	char message[8];
	message[0] = register_addr;
	TWI_MasterWriteRead(twi, ina226_addr, (uint8_t *) message, 1, 2);
	
	_delay_ms(20);
	
	return twi->readData;
}

/***********************************************************************
Changes INA226 settings to increase averaging + delay between conversion
This is for more accurate readings but slower
************************************************************************/
void high_INA_averaging(TWI_Master_t *twi, uint8_t ina226_addr)
{
	//INA_Write(twi, 0x00, 0x4E, 0x07, ina226_addr);
	INA_Write(twi, 0x00, 0x45, 0xFF, ina226_addr);
	
	_delay_ms(20);
}

void low_INA_averaging(TWI_Master_t *twi, uint8_t ina226_addr)
{
	//INA_Write(twi, 0x00, 0x48, 0x07, ina226_addr);
	//If no communication for too long then do long delay to triger a watch dog counter
	INA_Write(twi, 0x00, 0x45, 0xFF, ina226_addr);
	
	_delay_ms(20);
}

uint16_t get_voltage(TWI_Master_t *twi, bool averaged, uint8_t ina226_addr)
{
	register8_t* read_val;
	if(averaged)
	{
		read_val = INA_Read_Long(twi, 0x02, ina226_addr);
	}
	else
	{
		read_val = INA_Read_Short(twi, 0x02, ina226_addr);
	}
	_delay_ms(20);
	int voltage_bits = ((read_val[0] << 8) + (read_val[1]));
	float voltage = voltage_bits * 0.00125 * 2.00; //; // 1.25 mV/bit
	int voltage_int = voltage * 100;
	
	if(voltage_int > 8000 || voltage_int < 0)//if negative or thinks greater than 80V
	{ 
		voltage_int = 0;
	}
	
	if (read_val == 0xFFFFFFFF)
	{
		voltage_int = -1;
	}
		
	return voltage_int;
}

uint16_t get_current(TWI_Master_t *twi, bool averaged, uint8_t ina226_addr)
{
	register8_t* read_val;
	if(averaged)
	{
		read_val = INA_Read_Long(twi, 0x01, ina226_addr);
	}
	else
	{
		read_val = INA_Read_Short(twi, 0x01, ina226_addr);
	}
	_delay_ms(20);
	int shunt_voltage_bits = ((read_val[0] << 8) + (read_val[1]));
	float current = shunt_voltage_bits * 0.0000025 / 0.002; // * 2.5 uV / 2 milli Ohm
	int volatile current_int = current * 100;
	
	if(current_int > 8000 || current_int < 0)//if negative or thinks greater than 80A
	{ 
		current_int = 0;
	}

	if (read_val == 0xFFFFFFFF)
	{
		current_int = -1;
	}
	
	return current_int;
}

