/*
 * ina226.h
 *
 * Created: 4/3/2018 3:17:30 PM
 *  Author: ENG_3
 */ 


#ifndef INA226_H_
#define INA226_H_

void INA_Write(TWI_Master_t *twi, uint8_t register_addr, uint8_t data_byte_1, uint8_t data_byte_2, uint8_t ina226_addr);
register8_t * INA_Read_Short(TWI_Master_t *twi, uint8_t register_addr, uint8_t ina226_addr);
register8_t * INA_Read_Long(TWI_Master_t *twi, uint8_t register_addr, uint8_t ina226_addr);
uint16_t get_voltage(TWI_Master_t *twi, bool averaging, uint8_t ina226_addr);
uint16_t get_current(TWI_Master_t *twi, bool averaging, uint8_t ina226_addr);
void high_INA_averaging(TWI_Master_t *twi, uint8_t ina226_addr);
void low_INA_averaging(TWI_Master_t *twi, uint8_t ina226_addr);

#endif /* INA226_H_ */