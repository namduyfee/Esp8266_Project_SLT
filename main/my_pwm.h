
#ifndef TIMER_CONFIG
#define TIMER_CONFIG

#include "my_lib.h"


/*
 * To make a GPIOx to PWM only assign PWM_GPIO_X into any position in array g_gpio_pwm_channel[] (this array in file pwm-timer1.c)
 * Not use GPIO_NUM2 for pwm
 **/
#define CHANNEL_NOT_USED 100

#define GPIO_CHANNEL_0	GPIO_NUM_4
#define GPIO_CHANNEL_1	GPIO_NUM_5
#define GPIO_CHANNEL_2	GPIO_NUM_13
#define GPIO_CHANNEL_3	GPIO_NUM_14
#define GPIO_CHANNEL_4	GPIO_NUM_12
#define GPIO_CHANNEL_5	GPIO_NUM_15
#define GPIO_CHANNEL_6	GPIO_NUM_16
#define GPIO_CHANNEL_7	GPIO_NUM_0



#define PWM_PERIOD 1000	// 1khz
#define MAX_NUM_CHANNEL 8

#define	TOTAL_GPIO_MCU 18

#define DEFAUL_DUTY 40

typedef struct pwm_info
{
	uint32_t gpio_channel[MAX_NUM_CHANNEL];
	uint32_t duty[MAX_NUM_CHANNEL];
	
	uint8_t num_channel_en;					
	
	
} Pwm_Typedef;

/********* prototype ***********/

void start_pwm(void);
void set_duty_pwm(uint32_t channel_num, uint32_t duty);
void set_duties_pwm(void);
extern Pwm_Typedef Pwm;

#endif