#ifndef TIMER_CONFIG
#define TIMER_CONFIG

#include "my_lib.h"


#define MAX_CHANNEL 20
#define DEFAUL_DUTY 30

#define RELOAD_DATA_PWM 100

#define PWM_CHANNEL0 0
#define GPIO_PWM_CHANNEL0 2	// D4

#define PWM_CHANNEL1 1
#define GPIO_PWM_CHANNEL1 4	// D2

#define PWM_CHANNEL2 2
#define GPIO_PWM_CHANNEL2 5 // D1

#define PWM_CHANNEL3 3
#define GPIO_PWM_CHANNEL3 13 // D7

#define PWM_CHANNEL4 4
#define GPIO_PWM_CHANNEL4 14 // D5


/********* prototype ***********/
extern volatile uint32_t g_dutis[MAX_CHANNEL];

extern volatile uint32_t g_cnt_pwm;

void config_GPIO_PWM();
void config_Timer();

uint32_t precent_to_duty(uint32_t precent);


#endif
