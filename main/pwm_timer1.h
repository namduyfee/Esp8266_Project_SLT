#ifndef TIMER_CONFIG
#define TIMER_CONFIG

#include "my_lib.h"

/*
 * To make a GPIOx to PWM only assign PWM_PIN_GPIOx into any position in array g_gpio_pwm_channel[] (this array in file pwm-timer1.c) 
 **/

#define	TOTAL_GPIO_MCU 32

#define DEFAUL_DUTY 30

#define RELOAD_DATA_PWM 100

#define PWM_PIN_GPIO0 0
#define PWM_PIN_GPIO1 1
#define PWM_PIN_GPIO2 2			// D4
#define PWM_PIN_GPIO3 3
#define PWM_PIN_GPIO4 4			// D2

#define PWM_PIN_GPIO5 5			// D1
#define PWM_PIN_GPIO6 6
#define PWM_PIN_GPIO7 7
#define PWM_PIN_GPIO8 8
#define PWM_PIN_GPIO9 9

#define PWM_PIN_GPIO10 10
#define PWM_PIN_GPIO11 11
#define PWM_PIN_GPIO12 12
#define PWM_PIN_GPIO13 13		// D7
#define PWM_PIN_GPIO14 14		// D5

#define PWM_PIN_GPIO15 15
#define PWM_PIN_GPIO16 16
#define PWM_PIN_GPIO17 17



/********* prototype ***********/

void config_GPIO_PWM(void);
void config_Timer(void);

uint32_t precent_to_duty(uint32_t precent);

extern uint32_t g_gpio_pwm_channel[];
extern const uint32_t g_pwm_channel_len;

extern volatile uint32_t g_dutis[TOTAL_GPIO_MCU];

extern volatile uint32_t g_cnt_pwm;

#endif
