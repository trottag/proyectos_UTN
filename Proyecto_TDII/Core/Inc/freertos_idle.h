/*
 * freertos_idle.h
 *
 *  Created on: Jun 4, 2021
 *      Author: Florencia
 */

#ifndef INC_FREERTOS_IDLE_H_
#define INC_FREERTOS_IDLE_H_

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );


#endif /* INC_FREERTOS_IDLE_H_ */
