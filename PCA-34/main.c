#include "board.h"

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
	
	uint16_t temp_voltage;
	
    while (1)
    {
		if (rtc_counter % 500 == 0)
		{
			temp_voltage = get_voltage(&twiMaster, FALSE, INA_ADDRESS);
			if (abs(temp_voltage - voltage) > 5)
			{
				voltage = temp_voltage;
				update = TRUE;
			}
		}
		
		if (update)
		{
			voltage = get_voltage(&twiMaster, FALSE, INA_ADDRESS);
			display_current_voltage(current, voltage);
			update = FALSE;
		}
		
		if (voltage > 5000)
		{
			_delay_ms(500);
			display_danger();
			_delay_ms(500);
			update = TRUE;
		}
    }
}

ISR(TWI0_TWIM_vect)
{
	TWI_MasterInterruptHandler(&twiMaster);
}

ISR(RTC_CNT_vect)
{
	handle_encoder();
	rtc_counter++;
	RTC.INTFLAGS = RTC_OVF_bm;
}