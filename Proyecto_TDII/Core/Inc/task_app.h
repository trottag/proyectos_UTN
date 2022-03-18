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

#define NSAMPLES 	5


void xLcdTask( void *pvParameters );
void vTaskControl( void *pvParameters );
void vSenderTask_Temp( void *pvParameters );
void vSenderTask_Heart(void *pvParameters );
void xAdcTask( void *pvParameters );
void xPulsoTask( void *pvParameters );
#endif /* INC_TASK_APP_H_ */
