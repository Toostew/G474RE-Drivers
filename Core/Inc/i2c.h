/*
 * i2c.h
 *
 *  Created on: 5 Jul 2026
 *      Author: tooka
 */

#ifndef INC_I2C_H_
#define INC_I2C_H_

void i2c1_MPU_config_polling();
uint8_t i2c1_poll_read(uint8_t slave_addr, uint8_t reg_addr);
void i2c1_poll_write(uint8_t slaveAddr, uint8_t targetReg, uint8_t payload);



#endif /* INC_I2C_H_ */
