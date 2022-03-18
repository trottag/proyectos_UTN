/*
 * task.h
 *
 *  Created on:
 *      Author:
 */

#ifndef INC_TASK_APP_H_
#define INC_TASK_APP_H_

#include "stm32f4xx_hal.h"
#include "semphr.h"

#define INICIO 0
#define VALIDO 1
#define INICIO2 2
#define VALIDO2 3

void xLcdTask( void *pvParameters );
void vSenderTask( void *pvParameters );
void vTaskControl( void *pvParameters );
void vTaskBlinky( void *pvParameters );
void vTaskCount(void *pvParameters );

#endif /* INC_TASK_APP_H_ */
