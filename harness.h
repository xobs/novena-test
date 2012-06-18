#ifndef __HARNESS_H__
#define __HARNESS_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void harness_begin(char *name, int test_number);
void harness_error(uint32_t code, char *fmt, ...);
void harness_info(uint32_t code, char *fmt, ...);
void harness_debug(uint32_t code, char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* __HARNESS_H__ */
