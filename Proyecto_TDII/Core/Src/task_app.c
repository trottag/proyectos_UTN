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
#include "max30100_for_stm32_hal.h"


extern uint16_t _max30100_ir_sample[16];
uint32_t Rx_Data[30];

extern xQueueHandle xQueueADC,xQueue_teclas,xQueue_lcd;
extern UART_HandleTypeDef huart2;
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;



extern xQueueHandle xQueue_teclas, xQueueADC, xQueue_lcd,xQueue_lcd_p;
extern TaskHandle_t TaskHearthandler, TaskTemphandler, TaskAdcTaskhandler, TaskI2Chandler;
extern xSemaphoreHandle xSemNucleo, xSemPoncho,xSemSensor;
extern HD44780 lcd;


void xLcdTask( void *pvParameters )
{

	uint8_t estado=0;
	for ( ;; ){

		xQueueReceive( xQueue_lcd, &estado, portMAX_DELAY );

		if( (estado==0) || (estado==3) ){
			HD44780_init(&lcd);
			HD44780_put_str(&lcd, "SWP:Lector Pulso");
			HD44780_cursor_to(&lcd, 0, 1);
			HD44780_put_str(&lcd, "SWN:Lector Temp ");
		}
		if(estado==1){
			HD44780_cursor_to(&lcd, 0, 0);
			HD44780_put_str(&lcd, "                ");
			HD44780_cursor_to(&lcd, 0, 0);
			HD44780_put_str(&lcd, "Temp:           ");
			HD44780_cursor_to(&lcd, 0, 1);
			HD44780_put_str(&lcd, "                ");
		}
		if(estado==2){
			HD44780_cursor_to(&lcd, 0, 0);
			HD44780_put_str(&lcd, "                ");
			HD44780_cursor_to(&lcd, 0, 0);
			HD44780_put_str(&lcd, "Pulsos:         ");
			HD44780_cursor_to(&lcd, 0, 1);
			HD44780_put_str(&lcd, "                ");
		}
	}
}
/*-----------------------------------------------------------------------------------------*/
void xAdcTask( void *pvParameters )
{

	vTaskSuspend(NULL);
	uint16_t counts=0;
	uint32_t acum=0;
	uint16_t prom=0;
	uint16_t counter=0;
	float temp=0;
	char data[10];
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
	HAL_ADC_Start_IT(&hadc1);

	for( ;; )
	{
		xQueueReceive(xQueueADC, &counts, portMAX_DELAY);
		acum+=counts;
		counter++;
		if(counter==NSAMPLES){		//5 muestras
			prom=acum/NSAMPLES;
			acum=0;
			counter=0;
			temp = prom/100;	//divido por 100 debido a la lógica del LM35: Vo = Temperatura *100mV (Son 100 debido a que lo amplificamos 10 veces, sino son 10mV)
			sprintf(data,"%f", temp);
			HD44780_cursor_to(&lcd, 9, 0);
			HD44780_put_str(&lcd, "     ");
			HD44780_cursor_to(&lcd, 7, 0);
			HD44780_put_str(&lcd, data);
		}
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
void xPulsoTask( void *pvParameters )
{

	char data[16];
	uint8_t datos[16];
	xSemaphoreTake( xSemSensor, 0 );
	for( ;; ){

		xSemaphoreTake( xSemSensor, portMAX_DELAY );

		for(uint8_t i =0;i<16; i++)
			datos[i]=_max30100_ir_sample[i]/100;
		for(uint8_t i = 0; i< 16; i++)
			sprintf(data,"%u", datos[i]);
		HD44780_cursor_to(&lcd, 9, 0);
		HD44780_put_str(&lcd, "     ");
		HD44780_cursor_to(&lcd, 7, 0);
		HD44780_put_str(&lcd, data);

	}
}
/*-----------------------------------------------------------------------------------------*/
void vTaskControl( void *pvParameters )
{
	uint8_t cola =0, estado=0;

	for( ;; ){

		switch(estado){

		case 0:
			xQueueReceive( xQueue_teclas, &cola, portMAX_DELAY );
			if(cola==1){
				estado =1;
				xQueueSendToBack( xQueue_lcd, &cola, portMAX_DELAY );
				vTaskResume(TaskAdcTaskhandler);
			}

			if(cola==2){
				estado =2;
				xQueueSendToBack( xQueue_lcd, &cola, portMAX_DELAY );
				MAX30100_Resume();
				vTaskResume(TaskI2Chandler);
			}
			break;

		case 1:
			xQueueReceive( xQueue_teclas, &cola, portMAX_DELAY );
			if(cola==0){
				estado =0;
				xQueueSendToBack( xQueue_lcd, &cola, portMAX_DELAY );
				vTaskSuspend(TaskAdcTaskhandler);
			}
			break;

		case 2:
			xQueueReceive( xQueue_teclas, &cola, portMAX_DELAY );
			if(cola==3){
				estado =0;
				xQueueSendToBack( xQueue_lcd, &cola, portMAX_DELAY );
				MAX30100_Stop();
				vTaskSuspend(TaskI2Chandler);
			}
			break;

		}
	}
}


/*-----------------------------------------------------------------------------------------*/

void vSenderTask_Temp( void *pvParameters )
{

	uint8_t i=0,state;
	uint8_t estado = 0, cola = 0;
	xSemaphoreTake( xSemNucleo, 0 );

	for( ;; )
	{
		xSemaphoreTake( xSemNucleo, portMAX_DELAY );
		state=HAL_GPIO_ReadPin(GPIOC, Button_Nucleo_Pin);
		while((i<5)&&(state!=RESET)){
			state=HAL_GPIO_ReadPin(GPIOC, Button_Nucleo_Pin);
			i++;
			vTaskDelay(  5 / portTICK_RATE_MS );
		}

		switch(estado){
			case  0:
				if  (i == 5){
					estado = 1;
					i = 0;
					cola =1;
					xQueueSendToBack( xQueue_teclas, &cola, portMAX_DELAY );
				}
				break;

			case 1:
				if  (i == 5){
					estado = 0;
					i = 0;
					cola = 0;
					xQueueSendToBack( xQueue_teclas, &cola, portMAX_DELAY );
				}
				break;
		   }
		vTaskDelay(15 / portTICK_RATE_MS);
	}
}

/*-----------------------------------------------------------------------------------------*/
void vSenderTask_Heart(void *pvParameters )
{
	uint8_t i=0,button;
	uint8_t estado = 0, cola = 0;;
	xSemaphoreTake( xSemPoncho, 0 );

		for( ;; )
		{
			xSemaphoreTake( xSemPoncho, portMAX_DELAY );
			button=HAL_GPIO_ReadPin(GPIOA, Button_Poncho_Pin);
			while((i<5)&&(button!=RESET)){
				button=HAL_GPIO_ReadPin(GPIOA, Button_Poncho_Pin);
				i++;
				vTaskDelay(  5 / portTICK_RATE_MS );
			}

			switch(estado){
				case  0:
					if  (i == 5){
					i=0;
					estado = 1;
					cola =2;
					xQueueSendToBack( xQueue_teclas, &cola, portMAX_DELAY );
				}

				vTaskDelay( portTICK_RATE_MS);
				break;

			case 1:
				if  (i == 5){
					estado = 0;
					i = 0;
					cola =3;
					xQueueSendToBack( xQueue_teclas, &cola, portMAX_DELAY );
				}
			break;
			}
		vTaskDelay(15 / portTICK_RATE_MS);
	}
}
/*-----------------------------------------------------------------------------------------*/

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){

	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	if(GPIO_Pin==GPIO_PIN_13){
		xSemaphoreGiveFromISR( xSemNucleo, &xHigherPriorityTaskWoken );
	}else{
		if(GPIO_Pin==GPIO_PIN_0){
			xSemaphoreGiveFromISR( xSemPoncho, &xHigherPriorityTaskWoken );
		}else{
			//Error
		}
	}
	if(GPIO_Pin==GPIO_PIN_7){		// si el buffer del sensor de pulso esta lleno, interrumpe..
		xSemaphoreGiveFromISR( xSemSensor, &xHigherPriorityTaskWoken );
	}
	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

/*-----------------------------------------------------------------------------------------*/

