
#ifndef __APA102_H__
#define __APA102_H__

#include "pico.h"
#include "hardware/pio.h"
#include "hardware/dma.h"


typedef struct {
    unsigned int red : 8;
    unsigned int green : 8;
    unsigned int blue : 8;
    unsigned int brightness : 5;
    unsigned int unused : 3;
} APA102_LED;


class APA102 {
    public:
        APA102(uint strip_len) : APA102(strip_len, DEFAULT_APA102_CLK_PIN, DEFAULT_APA102_DIN_PIN) {};
        APA102(uint strip_len, uint8_t clk_pin, uint8_t din_pin) : APA102(strip_len, clk_pin, din_pin, pio0, 0) {};
        APA102(uint strip_len, uint8_t clk_pin, uint8_t din_pin, pio_hw_t *pio, uint sm) : APA102(strip_len, clk_pin, din_pin, pio, sm, DEFAULT_FREQUENCY) {};
        APA102(uint strip_len, uint8_t clk_pin, uint8_t din_pin, pio_hw_t *pio, uint sm, uint32_t frequency);
        ~APA102();

        APA102_LED *get_strip() { return (strip); };
        uint32_t get_strip_len() { return (strip_len); };
        void set_led(uint16_t led, uint8_t red, uint8_t green, uint8_t blue);
        void set_led(uint16_t led, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness);
        void update_strip();
        void clear_strip();

        static const uint32_t DEFAULT_FREQUENCY = 1000000;
        static const uint8_t DEFAULT_APA102_CLK_PIN = 2;
        static const uint8_t DEFAULT_APA102_DIN_PIN = 3;


    private:
        pio_hw_t *pio;
        uint pio_state_machine;
        uint32_t clk_freq;
        uint8_t clk_pin;
        uint8_t din_pin;
        uint strip_len;
        uint dma_pio_tx;
        dma_channel_config apa102_dma_channel_config;
        APA102_LED *strip;

        const uint8_t max_brightness = 31;

        uint32_t get_buffer_size();
};

#endif

