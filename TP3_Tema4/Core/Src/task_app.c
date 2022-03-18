/*
 * task.c
 *
 *  Created on:
 *      Author:
 */
/*-----------------------------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "task_app.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include "semphr.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "HD44780.h"

extern TaskHandle_t TaskAdcTaskhandler;
extern TaskHandle_t TaskAlarmTaskhandler;
extern TaskHandle_t TaskSendTaskhandler;
extern SemaphoreHandle_t xSemAlarm,xDmaTC_Tx;
extern xQueueHandle xQueueADC,xQueueTx,xQueueRx,xQueueCFG,xQueueCFGT;
extern UART_HandleTypeDef huart2;
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;
extern HD44780 lcd;
uint8_t rxdata;

/*-----------------------------------------------------------------------------------------*/
void xAdcTask( void *pvParameters )
{

	uint16_t counts=0;
	uint32_t acum=0;
	uint16_t prom=0;
	uint16_t tiempo,contado=1,seteo;
	uint16_t umbral=4000,counter=0;
	vTaskSuspend(NULL);

	HAL_GPIO_WritePin(GPIOA,Rele_Pin, GPIO_PIN_SET);
  	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	HAL_ADC_Start_IT(&hadc1);

	for( ;; )
	{
		xQueueReceive(xQueueADC, &counts, portMAX_DELAY);
		if(xQueueReceive(xQueueCFG, &umbral, 0)==pdTRUE){
			if(umbral>1800){
				umbral=1800;		//en caso de que el usuario se exceda del limite de parametrizacion, elijo setearlo con su max valor.
			}
		}
		if(xQueueReceive(xQueueCFGT, &tiempo, 0)==pdTRUE)
			seteo=tiempo;		//seteo del tiempo de envio por bluetooth

		if((counts*10)>umbral){	//multiplico por 10 a counts para que quede en el mismo formato que umbral (xxxx)
			xSemaphoreGive(xSemAlarm);
			counts=0;

		}else{
			acum+=counts;
			counter++;
			if((counter==NSAMPLES) && (umbral!=0)){		//25 muestras
				prom=acum/NSAMPLES*10;		// multiplico por 10 para tener el formato de 4 caracteres
				acum=0;
				counter=0;
				if(contado>=seteo){
					xQueueSendToBack(xQueueTx,&prom,portMAX_DELAY);
					contado=1;
				}
				else
					contado++;
			}
		}
	}
}
/*-----------------------------------------------------------------------------------------*/
void xAlarmTask( void *pvParameters )
{

	xSemaphoreTake(xSemAlarm,0);
	for( ;; )
	{
		xSemaphoreTake(xSemAlarm,portMAX_DELAY);
		vTaskSuspend(TaskAdcTaskhandler);
		HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
		HAL_GPIO_WritePin(GPIOA, Rele_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, LED_Rojo_Pin, SET);	//Enciendo el buzzer (lo reemplazo por el encendido del Led rojo del poncho)
		vTaskDelay(60000/portTICK_RATE_MS);	//1 Minuto
		HAL_GPIO_WritePin(GPIOA, Rele_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOB, LED_Rojo_Pin, RESET);
		vTaskResume(TaskAdcTaskhandler);
		HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	}


}
/*-----------------------------------------------------------------------------------------*/
void xSendTask( void *pvParameters )
{
	uint16_t prom=0;
	uint8_t tx_buffer[4],len=0;
	tx_buffer[0]='#';
	xSemaphoreGive( xDmaTC_Tx );
	for( ;; )
	{
		xQueueReceive(xQueueTx,&prom,portMAX_DELAY);
		itoa(prom,(char*)(tx_buffer+1),10);
		len=strlen((const char *)tx_buffer);
		tx_buffer[len]='*';
		tx_buffer[len+1]='\n';
		xSemaphoreTake(xDmaTC_Tx,portMAX_DELAY);
		HAL_UART_Transmit_DMA(&huart2, (uint8_t*)tx_buffer, len+2);
	}


}
/*-----------------------------------------------------------------------------------------*/
void xReceiveTask( void *pvParameters )
{
	char display[4];
	uint8_t data=0;
	uint16_t umbral=0,tiempo;

	tramita Recibe;

	HAL_UART_Receive_IT(&huart2, &rxdata, 1);
	for( ;; )
	{
		xQueueReceive(xQueueRx,&data,portMAX_DELAY);
		Recibe=ParseFrame(data);
		umbral=atoi((const char*) Recibe.umbral);
		tiempo=atoi((const char*) Recibe.tiempo);

		      if((umbral) && (tiempo==0) ){
		        	  HD44780_cursor_to(&lcd, 8, 1);
		  		      HD44780_put_str(&lcd,"ERROR\0");
		  		    vTaskSuspend(TaskAdcTaskhandler);
		          }
		      else if((umbral) && (tiempo)){
		    	  HD44780_cursor_to(&lcd, 8, 1);
	  		      HD44780_put_str(&lcd,"    	");
		          sprintf(display, "%d", tiempo);
		          HD44780_cursor_to(&lcd, 9, 1);
		          HD44780_put_str(&lcd,(char*) display);
		          sprintf(display, "%d", umbral);
		       	  HD44780_cursor_to(&lcd, 7, 0);
		       	  HD44780_put_str(&lcd,(char*) display);
		       	  vTaskResume(TaskAdcTaskhandler);
		       	  xQueueSendToBack(xQueueCFG, &umbral, portMAX_DELAY);
		       	  xQueueSendToBack(xQueueCFGT, &tiempo, portMAX_DELAY);
		      	  }
			}


}

/*-----------------------------------------------------------------------------------------*/
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	if (huart->Instance == USART2)
	{
		xSemaphoreGiveFromISR(xDmaTC_Tx,&xHigherPriorityTaskWoken);
		portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );

			}
}
/*-----------------------------------------------------------------------------------------*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	if (huart->Instance == USART2)
	{
		xQueueSendToBackFromISR(xQueueRx,&rxdata,&xHigherPriorityTaskWoken);
		portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
		HAL_UART_Receive_IT(&huart2, &rxdata, 1);

			}
}
/*-----------------------------------------------------------------------------------------*/
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc){

	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	uint16_t adc_val;
	adc_val = HAL_ADC_GetValue(&hadc1);
	xQueueSendToBackFromISR(xQueueADC,&adc_val,&xHigherPriorityTaskWoken);
	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );

}
/*-----------------------------------------------------------------------------------------*/

tramita ParseFrame(uint8_t rxdata){

	static char rxbuffer[8]={0};
	static uint8_t index=0;
	static uint8_t status=__IDLE;
	int i =0;

	tramita recibido;
	recibido.umbral[5]=0;
	recibido.tiempo[3]=0;
	switch(status){
		case __IDLE:
			if(rxdata=='@'){
				index=0;
				status=__NOIDLE;
			}
			break;
		case __NOIDLE:
			if(rxdata=='*'){
				if(index>0){
					for(i=0;i<4;i++)
						recibido.umbral[i] =rxbuffer[i];
					for(i=0;i<3;i++)
						recibido.tiempo[i] =rxbuffer[i+4];
				}
				status=__IDLE;
				}else{
					rxbuffer[index++]=rxdata;
					if(index>7)
						status=__IDLE;
				}
			break;
	}
	return recibido;

}
