/*
 * interrupt.c
 *
 *  Created on: 6 Jul 2026
 *      Author: tooka
 */

#include "main.h"

//NOTE: As good practice, interrupt handlers are usually very short and non-blocking


//configure interrupts for DMA channel 1 and I2C1
void interrupt_DMA_Config(){
	//enable DMA Transfer Complete Interrupt in NVIC
	NVIC_SetPriority(DMA1_Channel1_IRQn, 5); // Set appropriate priority
	NVIC_EnableIRQ(DMA1_Channel1_IRQn);

	//enable I2C1 Global Interrupt in NVIC (for handling the STOP flag)
	NVIC_SetPriority(I2C1_EV_IRQn, 5);
    NVIC_EnableIRQ(I2C1_EV_IRQn);
}

//this function runs upon interrupt on DMA1 Channel 1
//it matches the .weak declaration
void DMA1_Channel1_IRQHandler(){
	if(DMA1->ISR & (1 << 1)){//check Transfer complete flag
		DMA1->IFCR |= (0b1111 << 0); //clear all 4 DMA1C1 flags
		DMA1_Channel1->CCR &= ~(1 << 0); //disable the channel
		I2C1->CR1 &= ~(1 << 15); //remove RXDMAEN from I2C1.

		I2C1->CR1 |= (1 << 5); //enable STOPIE for I2C1
		//we enable it now instead of setting it earlier so that we GUARANTEE we are only catching STOP flags
		//that occur AFTER DMA1 Channel 1 Transfer completes.
	}
}

//this function runs on interrupt on I2C1, particularly STOPF detection
//also matches the .weak
void I2C1_EV_IRQHandler(){
	if(I2C1->ISR & (1 << 5)){ //check STOPF
		I2C1->ICR |= (1 << 5); //clear I2C1 STOPF
		I2C1->CR1 &= ~(1 << 5); //disable STOPIE for I2C1

		//past this point, we absolutely guarantee that the transaction is over.
		readStatus = 1;
	}


}
