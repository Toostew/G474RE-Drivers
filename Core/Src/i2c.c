/*
 * i2c.c
 *
 *  Created on: 5 Jul 2026
 *      Author: tooka
 */

#include "main.h"


//configuration for I2C1, Polling, without DMA or interrupts for BME280
//configure RCC, GPIO
void i2c1_MPU_config_polling(){

	//enable clocks for I2C
	RCC->APB1ENR1 &= ~(1 << 21);
	RCC->APB1ENR1 |=  (1 << 21);

	//enable clocks for GPIOB
	RCC->AHB2ENR &= ~(1 << 1);
	RCC->AHB2ENR |=  (1 << 1);

	//change clock source for I2C1 to HSI16 (16 MHz)
	RCC->CCIPR &= ~(0b11 << 12);
	RCC->CCIPR |=  (0b10 << 12);


	//GPIOMODER set for I2C1
	GPIOB->MODER &= ~(0b11 << 16);
	GPIOB->MODER &= ~(0b11 << 18);
	GPIOB->MODER |= (0b10 << 16);
	GPIOB->MODER |= (0b10 << 18);

	//GPIO OTYPER set pull-drain
	GPIOB->OTYPER &= ~(1 << 8);
	GPIOB->OTYPER &= ~(1 << 9);
	GPIOB->OTYPER |=  (1 << 8);
	GPIOB->OTYPER |=  (1 << 9);

	//GPIO OSPEEDR set low speed (i2c running at 100 khz)
	GPIOB->OSPEEDR &= ~(0b11 << 16);
	GPIOB->OSPEEDR &= ~(0b11 << 18);

	//GPIO PUPDR set pull up for pull up resistors
	GPIOB->PUPDR &= ~(0b11 << 16);
	GPIOB->PUPDR &= ~(0b11 << 18);
	GPIOB->PUPDR |= (1 << 16);
	GPIOB->PUPDR |= (1 << 18);

	//GPIO set AF4 for I2C1 SDA and SCL
	//AFR is split into 2 32-bit arrays,AFRH is AFR[1]
	GPIOB->AFR[1] &= ~(0b1111 << 0);
	GPIOB->AFR[1] &= ~(0b1111 << 4);
	GPIOB->AFR[1] |= (4 << 0);
	GPIOB->AFR[1] |= (4 << 4);

	//set timing reg for 100KHz, following manual
	I2C1->TIMINGR = 0x00000000;
	I2C1->TIMINGR |= (0x3 << 28); //PRESC
	I2C1->TIMINGR |= (0x4 << 20); //SCLDEL
	I2C1->TIMINGR |= (0x2 << 16); //SDADEL
	I2C1->TIMINGR |= (0xF << 8); //SCLH
	I2C1->TIMINGR |= (0x13 << 0); //SCLL

	//enable I2C1
	I2C1->CR1 = 0x00000000;
	I2C1->CR1 |= (1 << 0); //pe enable

}

//I2C polling read without DMA or Interrupts
uint8_t i2c1_poll_read(uint8_t slave_addr, uint8_t reg_addr) {
    uint8_t reg_value = 0;

    //wait until bus is free
    while (I2C1->ISR & I2C_ISR_BUSY);


    // Clear CR2 configuration bits
    I2C1->CR2 = 0x00000000;

    // Set up to write 1 byte (the register address)
    I2C1->CR2 |= (slave_addr << 1); //target address, SADD
    I2C1->CR2 |= (1 << 16); //NBYTES


    //generate START
    I2C1->CR2 |= I2C_CR2_START;

    //Wait for TXIS to be ready, then send the register address
    while (!(I2C1->ISR & I2C_ISR_TXIS)) {
        if (I2C1->ISR & I2C_ISR_NACKF) {
        	I2C1->ICR |= I2C_ICR_NACKCF; //clear NACK Flag
        	return 0;
        }
    }
    I2C1->TXDR = reg_addr;

   //wait for I2C1 TC flag flipped
    while (!(I2C1->ISR & I2C_ISR_TC));

    //READ PHASE
    //create a new CR2 with NBYTES and AUTOEND so we dont have to generate a stop condition ourselves
    I2C1->CR2 &= ~((1 << 25) | (0b11111111 << 16) | (0b1111111111 << 0));
    I2C1->CR2 |= (slave_addr << 1); //register address
    I2C1->CR2 |= (1 << 16);			//set Number of bytes
    I2C1->CR2 |= I2C_CR2_RD_WRN;   // Set to Read Mode
    I2C1->CR2 |= I2C_CR2_AUTOEND;  // Safe to auto-stop after we get our data

    //generate repeated start
    I2C1->CR2 |= I2C_CR2_START;

    //wait for RXNE (Receive register not empty)
    while (!(I2C1->ISR & I2C_ISR_RXNE)) {
        if (I2C1->ISR & I2C_ISR_NACKF) {
        	I2C1->ICR |= I2C_ICR_NACKCF;
        	return 0;
        }
    }

    //Read the data from RXDR
    reg_value = I2C1->RXDR;

    //wait for STOP flag to set.
    //Note that TC only occurs when AUTOEND = 0, and sets when NBYTES = 0.
    //TC is the software way of knowing the RX/TX ended because all the bytes have been sent/received.
    //but that doesnt mean The transaction is over. for it to be formally over, a STOP needs to be issued so that
    //the bus is handed back. When this happens, the STOPF is triggered
    while (!(I2C1->ISR & (1 << 5)));
    I2C1->ICR |= (1 << 5); //clear the STOPF

    return reg_value;
}

//polling I2C polling write without DMA or Interrupts
void i2c1_poll_write(uint8_t slaveAddr, uint8_t targetReg, uint8_t payload){

	//check busy flag
	while(!(I2C1->ISR & (1 << 15)));

	I2C1->CR2 = 0x00000000; //clear CR2

	I2C1->CR2 |= ((1 << 25) // AUTOEND = 1
			| (2 << 16) // number of bytes = 2
			| (0 << 11) // 7 bit address mode
			| (0 << 10) // transfer direction = write
			| (slaveAddr << 1)); // SADD, peripheral target address

	I2C1->CR2 |= (1 << 13); //fire START condition

	//check TXIS. set to 1 when TXDR is empty, meaning we are free to populate it
	while(!(I2C1->ISR & (1 << 1))){
		//while TXDR isnt empty check for NACKS
		//ACKs requires the slave to pull SDA low in response, if nothing happens within a clock cycle, it is interpretated as NACK
		if(I2C1->ISR & (1 << 4)){
			I2C1->ISR &= ~(1 << 4); //clear I2C1 NACKF
			return;
		}
	}

	I2C1->TXDR = targetReg;

	while(!(I2C1->ISR & (1 << 1))){
			//while TXDR isnt empty check for NACKS
			//ACKs requires the slave to pull SDA low in response, if nothing happens within a clock cycle, it is interpretated as NACK
			if(I2C1->ISR & (1 << 4)){
				I2C1->ISR &= ~(1 << 4); //clear I2C1 NACKF
				return;
			}
	}
	I2C1->TXDR = payload;

	while(!(I2C1->ISR & (1 << 5))); //check for stop flag

    I2C1->ICR |= (1 << 5); //clear the STOPF
}
