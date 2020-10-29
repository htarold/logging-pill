#ifndef WI2C_H
#define WI2C_H
#include <stdint.h>
#include <libopencm3/stm32/i2c.h>

#define WI2C_EBUSY      -81
#define WI2C_EMASTER    -82
#define WI2C_EADDRESS   -83
#define WI2C_EDATAOUT   -84
#define WI2C_EDATAIN    -85
#define WI2C_ETXSTATUS  -86
#define WI2C_ETRANSFER  -87
#define WI2C_ENODEV     -88


extern int8_t wi2c_init(uint32_t i2c);
extern int8_t wi2c_write7_v1(uint32_t i2c, int addr,
                             uint8_t * data, size_t n);
extern int8_t wi2c_transfer7(uint32_t i2c, uint8_t addr,
                             uint8_t * w, size_t wn,
                             uint8_t * r, size_t rn);
#endif /* WI2C_H */
