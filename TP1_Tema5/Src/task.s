
// Task header
#include "task.h"

.extern FlagSystick,1

/*------------------------------------------------------------------*/
		.syntax		unified
	    .arch       armv7-m
		.cpu		cortex-m0
		.thumb

		.section	.text
		.align		2


/*------------------------------------------------------------------*/
		.global	taskInit
		.type	taskInit, %function
		// R0, R1: argument / result / scratch register
		// R2, R3: argument / scratch register
		// R4 - R7: varaible register
taskInit:
		PUSH	{LR}					// Save PC

		BL		ledAInit					// ledAInit();		//Listo
		BL		ledRInit					//ledRInit			//Listo
		BL		SystickInit					// SystickInit();
		BL		GpioInit										//Listo
		BL		ExtiInit

		MOVS	R0, #LEDoN
		BL		ledAWrite				// ledAWrite(LEDoFF);
		BL		ledRWrite				// ledRWrite(LEDoN);

taskInitEnd:

		POP		{PC}					// Return

		.size	taskInit, . - taskInit

/*------------------------------------------------------------------*/
		.global	taskLedUpdate
		.type	taskLedUpdate, %function
		// R0, R1: argument / result / scratch register
		// R2, R3: argument / scratch register
		// R4 - R7: varaible register
taskLedUpdate:
		PUSH	{LR}					// Save PC

		PUSH	{R0,R1,R2,R3}				// Save arguments R0(LEDON u LEDOFF) R1 Demora

//		MOVS 	R3,#1
//		CMP		R0,#LEDoFF
//		BEQ		fixDuty
		LDR		R2,=CntDuty
		LDR		R3,[R2]

fixDuty:

		CMP		R3,#1
		BNE		fixDuty2

		MOVS	R0, R0					// R0 <- 1rst argument
		BL		ledAWrite				// ledAWrite(LEDoN or LEDoFF);
		BL		ledRWrite

		MOVS	R0,R1
		BL		delaySysTick			// delay(LEDoNdELAY or LEDoFFdELAY);

fixDuty2:

		CMP		R3,#2
		BNE		taskLedUpdateEnd

		MOVS	R0, R0
		BL		ledAWrite2				// ledAWrite(LEDoN or LEDoFF);
		BL		ledRWrite2

		MOVS	R0,R1
		BL		delaySysTick


taskLedUpdateEnd:
		POP		{R0, R1,R2,R3}
		POP		{PC}					// Return

		.size	taskLedUpdate, . - taskLedUpdate
/*------------------------------------------------------------------*/

		.global	LEDsOn
		.type	LEDsOn, %function
		// R0, R1: argument / result / scratch register
		// R2, R3: argument / scratch register
		// R4 - R7: varaible register
LEDsOn:

		PUSH	{LR}
		PUSH	{R0,R1,R2,R3}

		BL		ledAWrite2
		BL		ledRWrite2

		POP		{R0,R1,R2,R3}
LEDsOnEnd:

		POP		{PC}					// Return

		.size	LEDsOn, . - LEDsOn

/*------------------------------------------------------------------*/

		.global	taskDutyUpdate
		.type	taskDutyUpdate, %function
		// R0, R1: argument / result / scratch register
		// R2, R3: argument / scratch register
		// R4 - R7: varaible register
taskDutyUpdate:
		PUSH	{LR}					// Save PC

		PUSH	{R0, R1, R2}				// Save arguments

		LDR    	R1,=FlagExti
		MOVS	R2,#1
		LDRB	R3,[R1]
		CMP	    R3,R2
		BNE 	noDuty
		EORS	R2,R2
		STRB	R2,[R1]


		LDR		R0,=CntDuty
		LDR		R1,[R0]
		ADDS	R1,R1,#1
		CMP		R1,#3		//comparo las veces que sumo.. si llego a 3 reinicio
		BNE		nofullDuty
		MOVS	R1,#1
nofullDuty:

		STR		R1,[R0]
noDuty:
		POP	   {R0, R1, R2}

taskDutyUpdateEnd:

		POP		{PC}					// Return

		.size	taskDutyUpdate, . - taskDutyUpdate


/*------------------------------------------------------------------*/
		.type	ledAInit, %function
		// R0, R1: argument / result / scratch register
		// R2, R3: argument / scratch register
		// R4 - R7: varaible register
ledAInit:
		PUSH	{LR}					// Save PC
		LDR		R0,	=#RCC_AHB1ENR		// Dirección de memoria para habilitar clk en GPIO
		LDR		R3, [R0]				// Leo el registro
		LDR		R1, =#GPIOAEN			//
		ORRS	R3, R1					//
		STR		R3, [R0]
		LDR		R0, =#PORTA_MODER
		LDR		R3, [R0]
		MOVS	R1, #0x01
		LSLS	R1, R1, #8
		ORRS	R3, R1
		STR		R3, [R0]				//Leo la configuración de GPIOA y pongo A4 como salida

ledAInitEnd:

		POP		{PC}					// Retun

		.size	ledAInit, . - ledAInit


/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/
		.type	ledRInit, %function
		// R0, R1: argument / result / scratch register
		// R2, R3: argument / scratch register
		// R4 - R7: varaible register
ledRInit:
		PUSH	{LR}					// Save PC
		LDR		R0,	=#RCC_AHB1ENR		// Dirección de memoria para habilitar clk en GPIO
		LDR		R3, [R0]				// Leo el registro
		LDR		R1, =#GPIOBEN			//
		ORRS	R3, R1					//
		STR		R3, [R0]
		LDR		R0, =#PORTB_MODER
		LDR		R3, [R0]
		MOVS	R1, #0x01
		LSLS	R1, R1, #0
		ORRS	R3, R1
		STR		R3, [R0]				//Leo la configuración de GPIOB y pongo B8 como salida

ledRInitEnd:

		POP		{PC}					// Retun

		.size	ledRInit, . - ledRInit


/*------------------------------------------------------------------*/
		.type	ledAWrite, %function
		// R0, R1: argument / result / scratch register
		// R2, R3: argument / scratch register
		// R4 - R7: varaible register
		// R0: LEDoN or LEDoFF
ledAWrite:
		PUSH	{LR}					// Save PC
		PUSH	{R0,R1,R2,R3}



ledAWriteOff:
		LDR		R3, =PORTA_BSRR
       	MOVS	R2,	#20			//Bit correspondiente para el apagado del led amarillo
       	MOVS	R1, #1
       	CMP		R0, #LEDoN
		BEQ		ledAWriteEnd
ledAWriteOn:
        MOVS	R2,	#4		//Bit a colocar un uno para que encienda el led

ledAWriteEnd:

		LSLS	R1,R1,R2
		STR		R1, [R3]
		POP		{R0,R1,R2,R3}
		POP		{PC}					// Return

		.size	ledAWrite, . - ledAWrite


/*------------------------------------------------------------------*/
		.type	ledRWrite, %function
		// R0, R1: argument / result / scratch register
		// R2, R3: argument / scratch register
		// R4 - R7: varaible register
		// R0: LEDoN or LEDoFF
ledRWrite:
		PUSH	{LR}					// Save PC
		PUSH	{R0,R1,R2,R3}
ledRWriteOn:

        LDR		R3, =PORTB_BSRR
        MOVS	R2,	#0		//Bit a colocar un uno para que encienda el led
        MOVS	R1, #1
		CMP		R0, #LEDoN
		BEQ		ledRWriteEnd

ledRWriteOff:
       	MOVS	R2,	#16		//Bit correspondiente para el apagado del led rojo

ledRWriteEnd:

		LSLS	R1,R1,R2
		STR		R1, [R3]
		POP		{R0,R1,R2,R3}
		POP		{PC}					// Return

		.size	ledRWrite, . - ledRWrite
/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/
		.type	ledAWrite2, %function
		// R0, R1: argument / result / scratch register
		// R2, R3: argument / scratch register
		// R4 - R7: varaible register
		// R0: LEDoN or LEDoFF
ledAWrite2:
		PUSH	{LR}					// Save PC
		PUSH	{R0,R1,R2,R3}
ledAWriteOn2:
		LDR		R3, =PORTA_BSRR
       	MOVS	R2,	#4		//Bit a colocar un uno para que encienda el led
       	MOVS	R1, #1
		LSLS	R1,R1,R2
		STR		R1, [R3]
		POP		{R0,R1,R2,R3}
		POP		{PC}					// Return

		.size	ledAWrite2, . - ledAWrite2
/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/
		.type	ledRWrite2, %function
		// R0, R1: argument / result / scratch register
		// R2, R3: argument / scratch register
		// R4 - R7: varaible register
		// R0: LEDoN or LEDoFF
ledRWrite2:
		PUSH	{LR}					// Save PC
		PUSH	{R0,R1,R2,R3}
ledRWriteOn2:
        LDR		R3, =PORTB_BSRR
        MOVS	R2,	#0		//Bit a colocar un uno para que encienda el led
        MOVS	R1, #1
		LSLS	R1,R1,R2
		STR		R1, [R3]
		POP		{R0,R1,R2,R3}
		POP		{PC}					// Return
		.size	ledRWrite2, . - ledRWrite2
/*------------------------------------------------------------------*/


		.type	SystickInit, %function
		// R0, R1: argument / result / scratch register
		// R2, R3: argument / scratch register
		// R4 - R7: varaible register
SystickInit:
		PUSH	{LR}					// Save PC
		PUSH	{R0,R1}

		LDR		R0,=16000				// Valor a Cargar en Systick 1mseg Fclock=16Mhz => LOAD=16000
		LDR		R1, =STK_LOAD			//
		STR		R0, [R1]				//

		EORS	R0,R0
		LDR		R1,=STK_VAL				// VAL=0
		STR		R0, [R1]

		LDR		R0,=0x07				// CTRL=0x07	Start Systick,  Interrupt Enable
		LDR		R1,=STK_CTRL
		STR		R0, [R1]

		POP		{R0,R1}
SystickInitEnd:

		POP		{PC}					// RetuRn

		.size	SystickInit, . - SystickInit

/*------------------------------------------------------------------*/
		.type	GpioInit, %function
		// R0, R1: argument / result / scratch register
		// R2, R3: argument / scratch register
		// R4 - R7: varaible register
GpioInit:
		PUSH	{LR}					// Save PC

		PUSH	{R0,R1,R2,R3}

		LDR		R0,	=#RCC_AHB1ENR		// Dirección de memoria para habilitar clk en GPIO
		LDR		R3, [R0]				// Leo el registro
		LDR		R2, =#GPIOAEN			// Bits para habilitar PORTA
		ORRS	R3, R2					// Habilito los bits en cuestión
		STR		R3, [R0]				// Escribo el registro.


		MOVS	R2,#1
		//LSLS	R2, R2, #18				// 1<<18 ->01 pullup p	PB9
		LDR		R0,=PORTA_PUPDR
		LDR		R1, [R0]				//
		ORRS	R1, R2					//PORTB_PUPDR|1<<0
		STR		R1,[R0]
		POP		{R0,R1,R2,R3}
GpioInitEnd:

		POP		{PC}					// RetuRn

		.size	GpioInit, . - GpioInit

/*------------------------------------------------------------------*/
		.type	ExtiInit, %function
		// R0, R1: argument / result / scratch register
		// R2, R3: argument / scratch register
		// R4 - R7: varaible register
ExtiInit:
		PUSH	{LR}					// Save PC
		PUSH	{R0,R1,R2,R3}

		LDR		R0,=RCC_APBR2ENR	// System Config Controller Clock Enable
		LDR 	R1, [R0]
		LDR		R2,=0x4000			// RCC_APBR2ENR bit 14
		ORRS	R1,R2
		STR		R1,[R0]

		LDR		R0,=SYSCFG_EXTI_0
		LDR 	R1, [R0]
		LDR		R2,=0x0			// EXTI0  0000: PA[0] pin
		ORRS	R1,R2
		STR		R1,[R0]

		LDR		R0,=EXTI_IMR		// Unmask Bit 0			// Unmask Bit 9
		MOVS	R1,#1
		//LSLS	R1, R1, #0
		STR		R1,[R0]				//Unmmask Bit 0			// Unmask Bit 9

		LDR		R0,=EXTI_RTSR
		MOVS	R1,#1
		//LSLS	R1, R1, #0
		STR		R1,[R0]				//Rising Edge Enabled bit 0



		LDR		R0, =NVIC_BASE 		// NVIC_ISER[0]
		LDR		R1,[R0]				// R1=NVIC_ISER[0]
		MOVS	R3,#1
		LSLS	R2,R3,#6			// R2=1<<0 (EXTI3-0=NVIC IRQ40)
		ORRS	R1,R2				//R1=R1|R2
		STR		R1,[R0]				// NVIC_ISER[1]=R1
									// EXTI3-0 NVIC Enabled
		POP		{R0,R1,R2,R3}

ExtiInitEnd:

		POP		{PC}					// RetuRnExtiInit

		.size	ExtiInit, . - ExtiInit

/*------------------------------------------------------------------*/
		.type	delaySysTick, %function
		// R0, R1: argument / result / scratch register
		// R2, R3: argument / scratch register
		// R4 - R7: varaible register
		// R0:	Delay R0x1ms Base Time
delaySysTick:
		PUSH	{LR}					// Save PC
		PUSH	{R0,R1,R2,R3}

delaySysLoop:
		LDR    	R1,=FlagSystick
		MOVS	R2,#1
		LDRB	R3,[R1]
		CMP	    R3,R2
		BNE 	delaySysLoop

		EORS	R2,R2
		STRB	R2,[R1]

		SUBS 	R0, #1
		BNE 	delaySysLoop
		POP		{R0,R1,R2,R3}
		POP		{PC}					// Return

delaySysTickEnd:

		.size	delaySysTickEnd, . - delaySysTickEnd
/*------------------------------------------------------------------*/

		.end
// .end marks the end of the assembly file. as does not process anything in
// the file past the .end directive.

/*------------------------------------------------------------------*-
  ---- END OF FILE -------------------------------------------------
-*------------------------------------------------------------------*/
