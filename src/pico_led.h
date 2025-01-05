
#ifndef __PICO_LED_H__
#define __PICO_LED_H__

#ifdef cplusplus
extern "C" {
#endif

int pico_led_init(void);
void pico_set_led(bool led_on);

#ifdef cplusplus
}
#endif

#endif
