#ifndef __I2C_H__
#define __I2C_H__
#include <stdint.h>
int read_fpga(uint8_t start_reg, void *buffer, int bytes);
int write_fpga(uint8_t start_reg, void *buffer, uint32_t bytes);
int set_fpga(uint8_t reg, uint8_t val); /*Convenience function for write_fpga*/
uint8_t get_fpga(uint8_t reg);
int sync_fpga(void);

uint32_t read_adc(uint32_t channel);
#endif /* __I2C_H__ */
