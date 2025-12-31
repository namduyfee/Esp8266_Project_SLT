
#include "my_pwm.h"

/********* global variables ***********/

void my_pwm_start(Pwm_Typedef* Pwm)
{
	
	for (int i = 0; i < MAX_NUM_CHANNEL; i++)
	{
		if (Pwm->gpio_channel[i] == CHANNEL_NOT_USED)
			break;
		Pwm->duty[i] = DEFAUL_DUTY + 100 * i;
		Pwm->num_channel_en++;
	}
	
	if (Pwm->num_channel_en != 0)
	{
		
		Pwm->duty[Pwm->num_channel_en - 1] = 1000;
		
		pwm_init(PWM_PERIOD, Pwm->duty, Pwm->num_channel_en, Pwm->gpio_channel);
		pwm_start();
	}
	
	else
	{
		
	}
}

void set_duty_pwm(Pwm_Typedef* Pwm, uint32_t channel_num, uint32_t duty)
{
	if (channel_num >= Pwm->num_channel_en || Pwm->num_channel_en == 0)
	{
		return;
	}

	Pwm->duty[channel_num] = duty;
	
	pwm_set_duty(channel_num, Pwm->duty[channel_num]);
	pwm_start();
}

void set_duties_pwm(Pwm_Typedef* Pwm, uint32_t duty)
{
	if (Pwm->num_channel_en == 0)
		return;
	else
	{
		for (int i = 0; i < Pwm->num_channel_en; i++)
		{
			Pwm->duty[i] = duty; 
		}
		pwm_set_duties(Pwm->duty);
		pwm_start();
	}
}

