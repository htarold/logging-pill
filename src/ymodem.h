#ifndef YMODEM_H
#define YMODEM_H
#include <stdint.h>
extern int8_t ymodem_send(char * fn);
extern uint16_t ymodem_crc_update(uint16_t crc, uint8_t b);
extern int8_t ymodem_add(char b);
extern int8_t ymodem_flush(void);
extern int8_t ymodem_end(void);
#endif /* YMODEM_H */
