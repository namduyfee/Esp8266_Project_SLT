
#ifndef TIMER_CONFIG
#define TIMER_CONFIG

#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_event.h"
#include "esp_log.h"


#include "driver/pwm.h"

/*
 * To make a GPIOx to PWM only assign PWM_GPIO_X into any position in array g_gpio_pwm_channel[] (this array in file pwm-timer1.c)
 * Not use GPIO_NUM2 for pwm
 **/
#define CHANNEL_NOT_USED 100

#define GPIO_CHANNEL_0	GPIO_NUM_4		// D2
#define GPIO_CHANNEL_1	GPIO_NUM_5		// D1
#define GPIO_CHANNEL_2	GPIO_NUM_13		// D7
#define GPIO_CHANNEL_3	GPIO_NUM_14		// D5
#define GPIO_CHANNEL_4	GPIO_NUM_12		// D6
#define GPIO_CHANNEL_5	GPIO_NUM_15		// D8
#define GPIO_CHANNEL_6	GPIO_NUM_16		// D0
#define GPIO_CHANNEL_7	CHANNEL_NOT_USED		// D3
//#define GPIO_CHANNEL_7	GPIO_NUM_0		// D3 



#define PWM_PERIOD 1000	// 1khz
#define MAX_NUM_CHANNEL 8

#define	TOTAL_GPIO_MCU 18

#define DEFAUL_DUTY 40

#define MAX_DUTY 255

typedef struct pwm_info
{
	uint32_t gpio_channel[MAX_NUM_CHANNEL];
	uint8_t duty[MAX_NUM_CHANNEL];
	uint32_t period[MAX_NUM_CHANNEL];
	uint8_t num_channel_en;					
	
	
} Pwm_Typedef;

/********* prototype ***********/

void my_pwm_start(Pwm_Typedef* Pwm); 
void set_duty_pwm(Pwm_Typedef* Pwm, uint32_t channel_num, uint8_t duty); 
void set_duties_pwm(Pwm_Typedef* Pwm, uint8_t* duty, uint8_t len); 
uint32_t duty_to_period(uint8_t duty); 
#endif