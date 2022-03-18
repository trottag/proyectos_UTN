// Project header

#include "task.h"
// ------ Public functions -----------------------------------------

		.extern		taskInit, taskUpdate


/*------------------------------------------------------------------*/
		.section	.bss

		.comm	mainLoopCnt,4
		.comm	taskCnt,4
		.comm	FlagSystick,1
		.comm	FlagExti,1
		.comm 	CntDuty,1
/*------------------------------------------------------------------*/
		.syntax		unified
	    .arch       armv7-m
		.cpu		cortex-m0
		.thumb

		.section	.text
		.align		2
/*------------------------------------------------------------------*/
 		.global		main
		.type		main, %function

main:
		PUSH	{LR}					// Save PC

    	BL		systemInit				// systemInit();

    	LDR		R1, =FlagSystick		// R1 <- address of FlagSystick
		MOVS	R0, #0					// R0 <- 0
		STRB	R0, [R1]      			// mem[R1] <- R0 => FlagSystick = 0;

		LDR		R1, =mainLoopCnt		// R1 <- address of mainLoopCnt
		MOVS	R0, #0					// R0 <- 0
		STR		R0, [R1]      			// mem[R1] <- R0 => mainLoopCnt = 0;

		LDR		R1, =taskCnt			// R1 <- address of taskCnt
		MOVS	R0, #0					// R0 <- 0
		STR		R0, [R1]      			// mem[R1] <- R0 => taskCnt = 0;

		LDR		R1, =FlagExti		// R1 <- address of FlagExti
		MOVS	R0, #0					// R0 <- 0
		STRB	R0, [R1]      			// mem[R1] <- R0 => FlagExti = 0;

		LDR		R1, =CntDuty		// R1 <- address of CntDuty
		MOVS	R0, #1					// R0 <- 0
		STRB	R0, [R1]      			// mem[R1] <- R0 => CntDuty = 1;

mainLoop:

		LDR		R1, =taskCnt			// R1 <- address of taskCnt
  		LDR		R0, [R1]				// R0 <- mem[R1]
		CMP		R0, #tasksTableParts
		BHI		taskCntError			// Jump if (R0 > tasksTableParts)


		MOVS	R2, #tasksTablePartSize // Task Dir + Arguments
		MULS	R0, R0, R2				// R0= TaskCount * 12 bytes

		LDR		R1, =tasksTable			//
		LDR		R2, [R1, R0]			// R2= TaskUpdate[TaskCount]


		MOVS	R3, R0					//	R3 <- Init Element
		ADDS	R3, #tasksTableElemSize // 	R3=R3+ 4 bytes
  		LDR		R0, [R1, R3]			//  R0=Argument 1


		ADDS	R3, #tasksTableElemSize // R3=R3+ 4 bytes
  		LDR		R1, [R1, R3]			//  R1=Argument 2


		BLX		R2

		LDR		R1, =taskCnt			// R1 <- address of taskCnt
  		LDR		R0, [R1]				// R0 <- mem[R1]
		ADDS	R0, R0, #1				// R0 <- R0 + 1
		STR		R0, [R1]    			// mem[R1] <- R0 => taskCnt++;
		CMP		R0, #tasksTableParts
		BLT		mainLoopContinue		// Jump if (R0 < tasksTableParts)

		MOVS	R0, #0					// R0 <- 0
		STR		R0, [R1]      			// mem[R1] <- R0 => taskCnt = 0;

mainLoopContinue:

		LDR		R1, =mainLoopCnt		// R1 <- address of mainLoopCnt
  		LDR		R0, [R1]				// R0 <- mem[R1]
		ADDS	R0, R0, #1				// R0 <- R0 + 1
		STR		R0, [R1]      			// mem[R1] <- R0 => mainLoopCnt++;

		B		mainLoop				// Continue forever

mainLoopEnd:							// We should never reach here

		POP		{PC}					// Return

taskCntError:							// We should never reach here
		B		taskCntError

		.size	main, . - main


/*------------------------------------------------------------------*/
		.global	systemInit
		.type	systemInit, %function
systemInit:
		// R0, R1: argument / result / scratch register
		// R2, R3: argument / scratch register
		PUSH	{LR}					// Save PC
										// HW & SW initialization here
		BL		taskInit				// taskInit();

		POP		{PC}					// Retun

		.size	systemInit, . - systemInit
/*------------------------------------------------------------------*/
		.global	SysTick_Handler
		.type	SysTick_Handler, %function

SysTick_Handler:

		PUSH	{LR}					// Save PC
		PUSH	{R0,R1,R2}


		LDR		R0,=FlagSystick
		MOVS	R1,#1
		STRB	R1,[R0]
		POP		{R0,R1,R2}
		POP		{PC}

SysTick_HandlerEnd:

		.size	SysTick_Handler, . - SysTick_Handler
/*------------------------------------------------------------------*/

		.global	EXTI0_IRQHandler
		.type	EXTI0_IRQHandler, %function
EXTI0_IRQHandler:

		PUSH	{LR}					// Save PC
		PUSH	{R0,R1,R2,R3}

		LDR		R0,=EXTI_PR
		LDR		R1,[R0]
		MOVS 	R3,#1
	//	LSLS	R2,R3,#13
		TST		R1,R2
		BEQ		DetectEnd

		ORRS	R1,R2
		STR		R1,[R0]

		LDR		R0,=FlagExti
		MOVS	R1,#1
		STRB	R1,[R0]

DetectEnd:
		POP		{R0,R1,R2,R3}
		POP		{PC}
EXTI0_IRQHandlerEnd:

		.size	EXTI0_IRQHandler, . - EXTI0_IRQHandler
/*------------------------------------------------------------------*/
		.global	tasksTable
		.type	tasksTable, %object
		// Table of tasks
tasksTable:
		.word	taskLedUpdate
taskTableElement:
		.word	LEDoFF
		.word	LEDoFFdELAY
tasksTable1:

		.word	taskLedUpdate
		.word	LEDoN
		.word	LEDoNdELAY

		.word	taskDutyUpdate
		.word	0
		.word	0

tasksTableEnd:

		.size	tasksTable, . - tasksTable

		.equ	tasksTableSize, (tasksTableEnd - tasksTable)
		.equ	tasksTablePartSize, (tasksTable1 - tasksTable)
		.equ	tasksTableParts, (tasksTableSize/tasksTablePartSize)

		.equ	tasksTableElemSize, (taskTableElement - tasksTable)


		.end
// .end marks the end of the assembly file. as does not process anything in
// the file past the .end directive.

/*------------------------------------------------------------------*-
  ---- END OF FILE -------------------------------------------------
-*------------------------------------------------------------------*/
