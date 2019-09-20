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
		if (voltage > 200 && rtc_counter_1_2)
	    {
		    set_current_mode_on = set_current_mode ? 1 : 2;
		    display_danger();
		    _delay_ms(500);
			rtc_counter_1_2 = FALSE;
	    }
		else
		{
			if (set_current_mode_on == 1)
			{
				lcd_command(CLEAR_DISPLAY);
				lcd_command(FUNCTION_SET | 0b0000111100);
				clear_lcd_update();
				display_set_current();
				set_current_mode_on = 0;
			}
			else if (set_current_mode_on == 2)
			{
				lcd_command(CLEAR_DISPLAY);
				lcd_command(FUNCTION_SET | 0b0000111000);
				clear_lcd_update();
				display_current_voltage(current, voltage);
				// TODO - set DAC to the desired current
				eeprom_update_word((uint16_t*) 0x00, desired_current);
				desired_current_temp = desired_current;
				
				set_current_mode_on = 0;
			}
			else
			{
				if (set_current_mode)
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
					
					if (desired_current_temp != desired_current)
					{
						desired_current_temp = desired_current;
						display_set_current();
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
			}
		}
    }
}

ISR(TWI0_TWIM_vect)
{
	TWI_MasterInterruptHandler(&twiMaster);
}

// WARNING - DISPLAY COMMANDS WILL NOT BE EXECUTED IN THIS INTERRUPT 
//(since commands need to use i2c interrupt, which is impossible if 
//the program is in this interrupt)
ISR(RTC_CNT_vect)
{
	bool is_debounce_successful = TRUE;
	is_debounce_successful &= debounce(&PORTA_IN, ENCDR_SW, 10, 60);
	
	if (is_debounce_successful)
	{
		if(!(PORTA_IN & (1 << ENCDR_SW)))//if switch pin is low
		{
			encoder_sw_was_low = TRUE;
		}
		
		if((PORTA_IN & (1 << ENCDR_SW)) && encoder_sw_was_low)//if switch pin is high and was low
		{
			set_current_mode = !set_current_mode;
			set_current_mode_on = set_current_mode ? 1 : 2;
			rtc_idle_counter = rtc_counter;
			
			encoder_sw_was_low = FALSE;
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