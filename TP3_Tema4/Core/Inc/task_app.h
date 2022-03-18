/*
 * task.h
 *
 *  Created on:
 *      Author:
 */

#ifndef INC_TASK_APP_H_
#define INC_TASK_APP_H_

#define NSAMPLES 	25
#define __IDLE  		0
#define __NOIDLE 	1

void xReceiveTask( void *pvParameters );
void xSendTask( void *pvParameters );
void xAdcTask( void *pvParameters );
void xAlarmTask( void *pvParameters );

struct trama
{
	char umbral[5];	//[6]
	char tiempo[4];	//[5]
};
typedef struct trama tramita;

tramita ParseFrame(uint8_t rxdata);
#endif /* INC_TASK_APP_H_ */
