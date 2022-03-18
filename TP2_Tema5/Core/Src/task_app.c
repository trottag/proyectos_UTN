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
#include "stdio.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include "semphr.h"
#include "HD44780.h"


extern xQueueHandle xQueue_lcd2,xQueue_startstop,xQueue_contador,xQueue_lcd1,xQueue_frenado;
extern TaskHandle_t TaskBlinkyhandler;
extern TaskHandle_t TaskCounthandler;

extern xSemaphoreHandle xSemNucleo, xSemPoncho;
extern HD44780 lcd;
/*-----------------------------------------------------------------------------------------*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){

	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR( xSemNucleo, &xHigherPriorityTaskWoken );
	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );

}

void vTaskControl( void *pvParameters )
{
	uint8_t cola =0, p=0, e=0, estado=0;
	uint8_t t=0;
	uint8_t comp_contados =0;
	uint8_t cien_contados=0;
	char displayinfo1[3];
	char displayinfo2[4];

	for( ;; ){

		switch(estado){

		case 0:
			xQueueReceive( xQueue_startstop, &cola, portMAX_DELAY );
			p=cola;
			vTaskSuspend(TaskCounthandler);

			if (p==1){
				HAL_GPIO_WritePin(GPIOA, Rele_Pin, 1);
				HAL_GPIO_WritePin(GPIOB, Led_Rojo_Pin, 0);
				estado =1;
				vTaskDelay( 2000 / portTICK_RATE_MS );
				vTaskResume(TaskCounthandler);
			}
			break;

		case 1:
				if(xQueueReceive( xQueue_startstop, &t, 2000 / portTICK_RATE_MS )){
					vTaskResume(TaskCounthandler);

				e=t;
				if (e == 0){
					estado=0;
					HAL_GPIO_WritePin(GPIOA, Rele_Pin, 0);
					vTaskSuspend(TaskCounthandler);}

				if (e == 2){
					comp_contados++;
					e=0;
				}
				if(comp_contados==100){
					comp_contados =0;
					cien_contados= cien_contados+1;
					HAL_GPIO_WritePin(GPIOA, Led_Verde_Pin, 1);
					HAL_Delay(200);
					HAL_GPIO_WritePin(GPIOA, Led_Verde_Pin, 0);
				}

				sprintf(displayinfo1, "%d", cien_contados);
				sprintf(displayinfo2, "%d", comp_contados);
				xQueueSendToBack( xQueue_lcd1, displayinfo1, portMAX_DELAY );
				xQueueSendToBack( xQueue_lcd2, displayinfo2, portMAX_DELAY );

			}

				else{
				HAL_GPIO_WritePin(GPIOA, Rele_Pin, 0);
				vTaskResume(TaskBlinkyhandler);
			}
			break;

		}
	}
}


void xLcdTask( void *pvParameters )
{

	char displayinfo1[3];
	char displayinfo2[4];

	for( ;; )
	{

		xQueueReceive( xQueue_lcd1, displayinfo1, portMAX_DELAY );
		xQueueReceive( xQueue_lcd2, displayinfo2, portMAX_DELAY );

		HD44780_cursor_to(&lcd, 10, 0);
		HD44780_put_str(&lcd, "    ");
		HD44780_cursor_to(&lcd, 11, 0);
		HD44780_put_str(&lcd,(char*) displayinfo2);

		HD44780_cursor_to(&lcd, 10, 1);
		HD44780_put_str(&lcd, "   ");
		HD44780_cursor_to(&lcd, 11, 1);
		HD44780_put_str(&lcd, (char*) displayinfo1);


	}
}

void vSenderTask( void *pvParameters )
{

	uint8_t i=0,state;
	uint8_t estado = 0, cola = 0;

	xSemaphoreTake( xSemNucleo, 0 );

	for( ;; )
	{
		xSemaphoreTake( xSemNucleo, portMAX_DELAY );

		state=HAL_GPIO_ReadPin(GPIOC, Button_Nucleo_Pin);
		while((i<5)&&(state!=SET)){
			state=HAL_GPIO_ReadPin(GPIOC, Button_Nucleo_Pin);
			i++;
			vTaskDelay(  5 / portTICK_RATE_MS );
		}

		switch(estado){
			case  0:
				if  (i == 5){
					estado = 1;
					i = 0;
					cola = 1;
					xQueueSendToBack( xQueue_startstop, &cola, portMAX_DELAY );
				}
				else
					estado = 0;
				break;

			case 1:
				if  (i == 5){
					i = 0;
					estado = 0;
					cola = 0;
					xQueueSendToBack( xQueue_startstop, &cola, portMAX_DELAY );
				}
				else
					estado = 1;
				break;
		   }
		vTaskDelay(15 / portTICK_RATE_MS);	//agregado
	}
}

void vTaskCount(void *pvParameters )
{
	vTaskSuspend(NULL);
	uint8_t button=0;
	uint8_t i=0, t=0;
	uint8_t state = INICIO;

	for( ;; ){

		switch (state){

		case	INICIO:
			button = HAL_GPIO_ReadPin(GPIOA, Button_Poncho_Pin);
			while((i<5)&&(button!=SET)){
				button =HAL_GPIO_ReadPin(GPIOA, Button_Poncho_Pin);
				i++;
				vTaskDelay(5 / portTICK_RATE_MS);
			}

			if(i==5){
				state = VALIDO;
			}
			i= 0;
			break;

		case VALIDO:
			button =HAL_GPIO_ReadPin(GPIOA, Button_Poncho_Pin);
			while((i<5)&&(button==SET)){
				i++;
				vTaskDelay(5 / portTICK_RATE_MS);
				button =HAL_GPIO_ReadPin(GPIOA, Button_Poncho_Pin);
			}
			if (button == SET){
			t=1;
			state= INICIO2;
			button=0;
			}
			i=0;
			vTaskDelay( portTICK_RATE_MS);
			break;

		case	INICIO2:
			button = HAL_GPIO_ReadPin(GPIOA, Button_Poncho_Pin);
			while((i<5)&&(button!=SET)){
				button =HAL_GPIO_ReadPin(GPIOA, Button_Poncho_Pin);
				i++;
				vTaskDelay(5 / portTICK_RATE_MS);
			}

			if(i==5){
				state = VALIDO2;
			}
			i= 0;
		break;

		case VALIDO2:
			button =HAL_GPIO_ReadPin(GPIOA, Button_Poncho_Pin);
			while((i<5)&&(button==SET)){
				i++;
				vTaskDelay(5 / portTICK_RATE_MS);
				button =HAL_GPIO_ReadPin(GPIOA, Button_Poncho_Pin);
			}

			if(button==SET){
			t=2;
			state= INICIO;
		//	xQueueSendToBack( xQueue_contador, &t, portMAX_DELAY );
			xQueueSendToBack( xQueue_startstop, &t, portMAX_DELAY );
			button=0;
			}
			else
				t=0;
			i=0;
			t=0;
		break;
		}

	}
}

void vTaskBlinky( void *pvParameters )
{
	vTaskSuspend(NULL);

	for( ;; )
	{
		HAL_GPIO_WritePin(GPIOA,  Led_Amarillo_Pin, SET);
		HAL_Delay(60);
		HAL_GPIO_WritePin(GPIOA, Led_Amarillo_Pin, RESET);
		HAL_Delay(40);
	}
}
