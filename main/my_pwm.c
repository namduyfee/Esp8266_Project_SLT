
#include "my_pwm.h"

/********* global variables ***********/

void my_pwm_start(Pwm_Typedef* Pwm)
{
	
	for (int i = 0; i < MAX_NUM_CHANNEL; i++)
	{
		if (Pwm->gpio_channel[i] == CHANNEL_NOT_USED)
			break;
		
		Pwm->duty[i] = DEFAUL_DUTY + 20 * i;
		if (Pwm->duty[i] > 255)
			Pwm->duty[i] = 255;
		else if (Pwm->duty[i] < 0)
			Pwm->duty[i] = 0;
		
		Pwm->period[i] = duty_to_period(Pwm->duty[i]);
		
		Pwm->num_channel_en++;
	}
	
	if (Pwm->num_channel_en != 0)
	{
		
		Pwm->duty[Pwm->num_channel_en - 1] = 255;
		Pwm->period[Pwm->num_channel_en - 1] = duty_to_period(Pwm->duty[Pwm->num_channel_en - 1]);
		
		pwm_init(PWM_PERIOD, Pwm->period, Pwm->num_channel_en, Pwm->gpio_channel);
		pwm_start();
	}
	
	else
	{
		
	}
}

void set_duty_pwm(Pwm_Typedef* Pwm, uint32_t channel_num, uint8_t duty)
{
	if (channel_num >= Pwm->num_channel_en || Pwm->num_channel_en == 0)
	{
		return;
	}

	Pwm->duty[channel_num] = duty;
	Pwm->period[channel_num] = duty_to_period(Pwm->duty[channel_num]);
	
	pwm_set_duty(channel_num, Pwm->period[channel_num]);
	pwm_start();
}

void set_duties_pwm(Pwm_Typedef* Pwm, uint8_t* duty, uint8_t len)
{
	if (Pwm->num_channel_en == 0)
		return;
	else
	{
		
		for (int i = 0; i < (Pwm->num_channel_en < len ? Pwm->num_channel_en : len); i++)
		{
			Pwm->duty[i] = duty[i]; 
			Pwm->period[i] = duty_to_period(Pwm->duty[i]);
		}
		pwm_set_duties(Pwm->period);
		pwm_start();
	}
}

uint32_t duty_to_period(uint8_t duty)
{
	if (duty >= 255)
		return PWM_PERIOD;
	else if (duty <= 0)
		return 0;
	
	return (duty * PWM_PERIOD) / MAX_DUTY;
}

