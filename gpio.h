#ifndef __GPIO_H__
#define __GPIO_H__

int gpio_export(int gpio);
int gpio_unexport(int gpio);
int gpio_set_direction(int gpio, int is_output);
int gpio_set_value(int gpio, int value);
int gpio_get_value(int gpio);

#endif // __GPIO_H__
