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
	
	volatile bool set_current_blink_temp = TRUE;
    while (1)
    {
		if (set_current_mode)
		{
			if (rtc_counter_1_4)
			{
				set_current_blink = !set_current_blink;
				rtc_counter_1_4 = FALSE;
			}
			
			if (rtc_counter_1_2)
			{
				temp_voltage = get_voltage(&twiMaster, FALSE, INA_ADDRESS);
				if (abs(temp_voltage - voltage) > 5)
				{
					voltage = temp_voltage;
				}
				rtc_counter_1_2 = FALSE;
			}
			
			if (abs(temp_voltage - voltage) > 5 || abs(temp_current - current) > 5 || set_current_blink_temp != set_current_blink)
			{
				if (set_current_blink)
				{
					display_current_voltage(current, voltage);
				}
				else
				{
					display_voltage(voltage);
				}
				
				if (set_current_blink_temp != set_current_blink)
				{
					set_current_blink_temp = set_current_blink;
				}
				
				temp_current = current;
			}
		}
		else
		{
			if (rtc_counter_1_4)
			{
				temp_voltage = get_voltage(&twiMaster, FALSE, INA_ADDRESS);
				if (abs(temp_voltage - voltage) > 5)
				{
					voltage = temp_voltage;
					update = TRUE;
				}
				
				temp_current = get_current(&twiMaster, FALSE, INA_ADDRESS);
				if (abs(temp_current - current) > 5)
				{
					current = temp_current;
					update = TRUE;
				}
				
				rtc_counter_1_4 = FALSE;
			}
			
			if (update)
			{
				display_current_voltage(current, voltage);
				update = FALSE;
			}
		}
		
		if (voltage > 5000) // TODO - use counters
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
	bool is_debounce_successful = TRUE;
	is_debounce_successful &= debounce(&PORTA_IN, ENCDR_SW, 10, 60);
	
	if (is_debounce_successful)
	{
		if(!(PORTA_IN & (1 << ENCDR_SW)))//if switch pin is low
		{
			if (set_current_mode)
			{
				// TODO - set DAC to the desired current
			}
			set_current_mode = !set_current_mode;
			rtc_idle_counter = rtc_counter;
		}
	}
	
	if (set_current_mode)
	{
		handle_encoder();
	}
	if (rtc_counter % 250 == 0)
	{
		rtc_counter_1_4 = TRUE;
	}
	if (rtc_counter % 500 == 0)
	{
		rtc_counter_1_2 = TRUE;
	}
	rtc_counter++;
	RTC.INTFLAGS = RTC_OVF_bm;
}