#ifndef GPIO_CONFIG
#define	GPIO_CONFIG

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

#include "driver/gpio.h"

void config_input_pullup_gpio(void);
void config_GPIO_OUT(void);

/*======= LIST PIN OF GPIO_NUM_x ========*/

/*	GPIO_NUM_0		D3
 *	GPIO_NUM_1 
 *	GPIO_NUM_2		D4
 *	GPIO_NUM_3
 *	GPIO_NUM_4		D2
 *	GPIO_NUM_5		D1
 *	GPIO_NUM_6
 *	GPIO_NUM_7
 *	GPIO_NUM_8
 *	GPIO_NUM_9
 *	GPIO_NUM_10
 *	GPIO_NUM_11
 *	GPIO_NUM_12		D6
 *	GPIO_NUM_13		D7
 *	GPIO_NUM_14		D5
 *	GPIO_NUM_15		D8
 *	GPIO_NUM_16
 *	GPIO_NUM_17
 *
 **/


/*======= PWM PIN =========*/

// Not use GPIO_NUM_2 for pwm
#define CHANNEL_NOT_USED 100
#define GPIO_PWM_CHAN_0	GPIO_NUM_0
#define GPIO_PWM_CHAN_1	GPIO_NUM_1
#define GPIO_PWM_CHAN_2	GPIO_NUM_3
#define GPIO_PWM_CHAN_3	GPIO_NUM_4
#define GPIO_PWM_CHAN_4	GPIO_NUM_5
#define GPIO_PWM_CHAN_5	GPIO_NUM_13
#define GPIO_PWM_CHAN_6	GPIO_NUM_14
#define GPIO_PWM_CHAN_7	GPIO_NUM_15

/*======= PIN SELECT MASTER =======*/
#define BUT_SEL_MASTER GPIO_NUM_12

#define PRES_SEL_MASTER 0

#endif

