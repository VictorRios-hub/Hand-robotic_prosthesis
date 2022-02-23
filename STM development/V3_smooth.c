/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Auto-generated by STM32CubeIDE
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

#include "stm32f0xx.h"
#include "main.h"
#include "bsp.h"
#include "delay.h"

// Static functions

static void SystemClock_Config(void);

// Global variables

uint16_t in, old;
uint16_t pulse;

// Main program


int main()
{
	// Local variables
	uint16_t c1, c2;   // Filter coefs
	uint16_t x = 0;    // Filter input
	uint32_t y = 0;    // Filter output

	// Setup filter coefs
	c1 = 482;
	c2 = 3614;

	// Initialize PWM middle
	pulse = 1500;

	// Configure System Clock
	SystemClock_Config();

	// Initialize LED pin
	BSP_LED_Init();

	// Turn LED On
	BSP_LED_On();

	// Turn LED Off
	BSP_LED_Off();

	// Initialize and start time base
	BSP_TIMER_Timebase_Init();

	// Initialize Debug Console
	BSP_Console_Init();
	my_printf("Console ready!\r\n");

	// Initialize and start ADC on PC1
	BSP_ADC_Init();
	my_printf("ADC ready!\r\n");

	// Initialize Timer for delays
	BSP_DELAY_TIM_init();

	// Initialize Timer for PWM output
	BSP_TIMER_PWM_Init();


	// Main loop
	while(1)
	{
		// Start measure
		BSP_LED_On();

		// Hand command
		TIM1->CCR1 = pulse;

		// Input signal <-- ADC
		while ((ADC1->ISR & ADC_ISR_EOC) != ADC_ISR_EOC);
		in = ADC1->DR;

		// DEBUG
		//my_printf("ADC value = %d\r\n", in);

		// Filter stage
		x = in;
		y = (c1*x) + (c2*y);
		y = (y >>12U);		// Remove decimal part
		in = y;

		// DEBUG
		my_printf("ADC value = %d\r\n", in);

		// Treshold & command
		if (in >= 220)
		{
			if (pulse <= 2500 )
			pulse ++;				// Close hand
		}
		else if (in <= 160)
		{
			if (pulse >= 500 )		// Open hand
			pulse --;
		}
		else						// Smooth the movement
		{
			if (old >= 220)
			{
				if (pulse <= 2500 )
				pulse ++;
			}
			else if (old <= 160)
			{
				if (pulse >= 500 )
				pulse --;
			}
		}
		// Save old data
		old = in;

		// Stop measure
		BSP_LED_Off();

		// Wait for 2ms
		BSP_DELAY_TIM_ms(2);
	}
}

/*
 * 	Clock configuration for the Nucleo STM32F072RB board
 * 	HSE input Bypass Mode 			-> 8MHz
 * 	SYSCLK, AHB, APB1 				-> 48MHz
*  	PA8 as MCO with /16 prescaler 		-> 3MHz
 *
 *  Laurent Latorre - 05/08/2017
 */

static void SystemClock_Config()
{
	uint32_t	HSE_Status;
	uint32_t	PLL_Status;
	uint32_t	SW_Status;
	uint32_t	timeout = 0;

	timeout = 1000000;

	// Start HSE in Bypass Mode
	RCC->CR |= RCC_CR_HSEBYP;
	RCC->CR |= RCC_CR_HSEON;

	// Wait until HSE is ready
	do
	{
		HSE_Status = RCC->CR & RCC_CR_HSERDY_Msk;
		timeout--;
	} while ((HSE_Status == 0) && (timeout > 0));

	// Select HSE as PLL input source
	RCC->CFGR &= ~RCC_CFGR_PLLSRC_Msk;
	RCC->CFGR |= (0x02 <<RCC_CFGR_PLLSRC_Pos);

	// Set PLL PREDIV to /1
	RCC->CFGR2 = 0x00000000;

	// Set PLL MUL to x6
	RCC->CFGR &= ~RCC_CFGR_PLLMUL_Msk;
	RCC->CFGR |= (0x04 <<RCC_CFGR_PLLMUL_Pos);

	// Enable the main PLL
	RCC-> CR |= RCC_CR_PLLON;

	// Wait until PLL is ready
	do
	{
		PLL_Status = RCC->CR & RCC_CR_PLLRDY_Msk;
		timeout--;
	} while ((PLL_Status == 0) && (timeout > 0));

        // Set AHB prescaler to /1
	RCC->CFGR &= ~RCC_CFGR_HPRE_Msk;
	RCC->CFGR |= RCC_CFGR_HPRE_DIV1;

	//Set APB1 prescaler to /1
	RCC->CFGR &= ~RCC_CFGR_PPRE_Msk;
	RCC->CFGR |= RCC_CFGR_PPRE_DIV1;

	// Enable FLASH Prefetch Buffer and set Flash Latency
	FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;

	/* --- Until this point, MCU was still clocked by HSI at 8MHz ---*/
	/* --- Switching to PLL at 48MHz Now!  Fasten your seat belt! ---*/

	// Select the main PLL as system clock source
	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR |= RCC_CFGR_SW_PLL;

	// Wait until PLL becomes main switch input
	do
	{
		SW_Status = (RCC->CFGR & RCC_CFGR_SWS_Msk);
		timeout--;
	} while ((SW_Status != RCC_CFGR_SWS_PLL) && (timeout > 0));

	/* --- Here we go! ---*/

	/*--- Use PA8 as MCO output at 48/16 = 3MHz ---*/

	// Set MCO source as SYSCLK (48MHz)
	RCC->CFGR &= ~RCC_CFGR_MCO_Msk;
	RCC->CFGR |=  RCC_CFGR_MCOSEL_SYSCLK;

	// Set MCO prescaler to /16 -> 3MHz
	RCC->CFGR &= ~RCC_CFGR_MCOPRE_Msk;
	RCC->CFGR |=  RCC_CFGR_MCOPRE_DIV16;

	// Enable GPIOA clock
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

	// Configure PA8 as Alternate function
	GPIOA->MODER &= ~GPIO_MODER_MODER8_Msk;
	GPIOA->MODER |= (0x02 <<GPIO_MODER_MODER8_Pos);

	// Set to AF0 (MCO output)
	GPIOA->AFR[1] &= ~(0x0000000F);
	GPIOA->AFR[1] |=  (0x00000000);

	// Update SystemCoreClock global variable
	SystemCoreClockUpdate();
}
