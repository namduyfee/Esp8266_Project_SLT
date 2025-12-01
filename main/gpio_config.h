#ifndef GPIO_CONFIG
#define	GPIO_CONFIG

#include "my_lib.h"

void config_input_pullup_gpio(void);
void config_GPIO_PWM(void);

/*	GPIO_NUM_0
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

#define LED_WIFI GPIO_NUM_13

void config_input_pullup_gpio(void);

void config_GPIO_OUT(void);
#endif

