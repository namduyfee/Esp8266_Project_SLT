
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


#define PWM_PERIOD 1000	// 1khz
#define MAX_NUM_CHANNEL 8

#define	TOTAL_GPIO_MCU 18

#define DEFAUL_DUTY 70

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