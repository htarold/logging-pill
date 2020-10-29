#ifndef DELAY_H
#define DELAY_H
/*
  Not very accurate timing delays
 */
#ifndef MHZ
#define MHZ 72
#warning Using MHZ Default value
#endif

#define delay_us(us) \
{ uint32_t i; for (i = 1.33*MHZ*us/8; i > 0; i--) { __asm__("nop"); } }

#define delay_ms(ms) \
{ uint32_t i; for (i = ms; i > 0; i--) { delay_us(1000); } }

#endif /* DELAY_H */
