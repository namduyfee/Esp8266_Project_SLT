
#include "my_pwm.h"


/********* global variables ***********/

Pwm_Typedef Pwm = {
	.gpio_channel = {
		GPIO_CHANNEL_0,
		GPIO_CHANNEL_1,
		GPIO_CHANNEL_2,
		GPIO_CHANNEL_3,
		GPIO_CHANNEL_4, 
		GPIO_CHANNEL_5,
		GPIO_CHANNEL_6,
		GPIO_CHANNEL_7
	},
	.duty = {0, 0, 0, 0, 0, 0, 0, 0},
	.num_channel_en = 0,
};

void start_pwm(void)
{
	
	for (int i = 0; i < MAX_NUM_CHANNEL; i++)
	{
		if (Pwm.gpio_channel[i] == CHANNEL_NOT_USED)
			break;
		Pwm.duty[i] = DEFAUL_DUTY + 100 * i;
		Pwm.num_channel_en++;
	}
	
	if (Pwm.num_channel_en != 0)
	{
		
		Pwm.duty[Pwm.num_channel_en - 1] = 1000;
		
		pwm_init(PWM_PERIOD, Pwm.duty, Pwm.num_channel_en, Pwm.gpio_channel);
		pwm_start();
	}
	
	else
	{
		
	}
}

void set_duty_pwm(uint32_t channel_num, uint32_t duty)
{
	if (channel_num >= Pwm.num_channel_en || Pwm.num_channel_en == 0)
	{
		return;
	}

	Pwm.duty[channel_num] = duty;
	
	pwm_set_duty(channel_num, Pwm.duty[channel_num]);
	pwm_start();
}

void set_duties_pwm(void)
{
	if (Pwm.num_channel_en == 0)
		return;
	else
	{
		pwm_set_duties(Pwm.duty);
		pwm_start();
	}
}

