#ifndef TIMER_CONFIG
#define TIMER_CONFIG

#include "my_lib.h"

/*
 * To make a GPIOx to PWM only assign GPIOx into any position in array g_gpio_pwm_channel[] (this array in file pwm-timer1.c)
 **/

#define	TOTAL_GPIO_MCU 32

#define DEFAUL_DUTY 4

#define RELOAD_DATA_PWM 16

/********* prototype ***********/

void config_Timer(void);


extern uint32_t g_gpio_pwm_channel[];
extern const uint32_t g_pwm_channel_len;

extern volatile uint32_t g_dutis[TOTAL_GPIO_MCU];

extern volatile uint32_t g_cnt_pwm;

#endif
