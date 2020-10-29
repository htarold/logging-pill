#ifndef SENSORS_H
#define SENSORS_H
#include <stdint.h>
extern uint16_t sensors_bytes_per_record(void);
extern int sensors_init(void);
extern void sensors_print(void (*putc)(char), uint32_t addr);
extern uint32_t sensors_record(uint32_t addr);
#endif /* SENSORS_H */
