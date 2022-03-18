

#ifndef TASK_H_
#define TASK_H_

#define RCC_AHB1ENR 	0x40023830		// Registros para habilitar el clock de GPIOC
#define GPIOCEN			0x4				// bit para habilitar clk en GPIOC
#define GPIOAEN			0x1				// bit para habilitar clk en GPIA
#define GPIOBEN			0x2				// bit para habilitar clk en GPIB



#define PORTA_MODER		0x40020000		// Registro para setear la función de cada pin del GPIO
#define PORTA_BSRR		0x40020018		// Registro para escritura tipo SET - RESET

#define PORTB_MODER		0x40020400		//GPIOB
#define PORTB_BSRR		0x40020418



#define PORTA_IDR		0x40020810		// Registro de entrada
#define PORTA_PUPDR		0x4002000C		// PullUp-DownReg

#define SYSCFG			0x40013800
#define SYSCFG_EXTI_0 	(SYSCFG + 0x08UL)		//listo

#define EXTI_BASE            (0x40013C00)
#define EXTI_IMR             (0x40013C00)
#define EXTI_RTSR            (EXTI_BASE  + 0x08UL)
#define EXTI_PR              (EXTI_BASE  + 0x14UL)
/***************************************************/
#define RCC_APBR2ENR		0x40023844


#define SCS_BASE            (0xE000E000UL)
#define NVIC_BASE           (SCS_BASE +  0x0100UL)

#define STK_CTRL		0xE000E010
#define	STK_LOAD		0xE000E014
#define	STK_VAL			0xE000E018

#define CNTFLAG_MASK	0x10000


#define false	0
#define true	1

#define LEDdELAY	167
#define LEDoNdELAY	LEDdELAY
#define LEDoFFdELAY	167

#define LEDoFF	false
#define LEDoN	true

#endif /* TASK_H_ */
