
#include <cstring>
#include <stdio.h>
#include "apa102.h"
#include "apa102.pio.h"


APA102::APA102(uint strip_len, uint8_t clk_pin, uint8_t din_pin, pio_hw_t *pio, uint sm, uint32_t frequency) :
    strip_len(strip_len),
    din_pin(din_pin),
    clk_pin(clk_pin),
    clk_freq(frequency),
    pio_state_machine(sm),
    pio(pio)
{
    printf("CONSTRUCTING APA102\n");
    printf("buffer size=%d\n", get_buffer_size());
    strip = new APA102_LED[get_buffer_size()];
    printf("strip=%08x\n", strip);

    // Set start and end frames per APA102 protocol
    memset(strip, 0, sizeof(APA102_LED));
    memset(&strip[strip_len + 1], 0xff, sizeof(APA102_LED));

    // All LEDs off
    clear_strip();

    // Set up PIO state machine
    uint offset = pio_add_program(pio, &apa102_mini_program);
    apa102_mini_program_init(pio, pio_state_machine, offset, clk_freq, clk_pin, din_pin);

    printf("pio offset=%d\n", offset);

    // Point DMA to the PIO state machine
    dma_pio_tx = dma_claim_unused_channel(true);
    apa102_dma_channel_config = dma_channel_get_default_config(dma_pio_tx);
    channel_config_set_transfer_data_size(&apa102_dma_channel_config, DMA_SIZE_32);
    channel_config_set_dreq(&apa102_dma_channel_config, pio_get_dreq(pio, pio_state_machine, true));
    printf("dma channel=%d\n", dma_pio_tx);
};


void APA102::clear_strip() {
    for(int i = 1; i < strip_len; i++) {
        strip[i].unused = 0x7;   // datasheet says these bits should be all ones
        strip[i].brightness = 0;
        strip[i].red = 0;
        strip[i].green = 0;
        strip[i].blue = 0;
    }
}


uint32_t APA102::get_buffer_size() {
    // First and last four bytes are the start and end frames per APA102 protocol
    return ((strip_len + 2) * sizeof(APA102_LED));
}


void APA102::update_strip() {
    dma_channel_configure(dma_pio_tx, 
                          &apa102_dma_channel_config,
                          &pio->txf[pio_state_machine],  // write to PIO TX FIFO
                          strip,                         // read from LED buffer
                          (get_buffer_size() / 4) - 1,   // 32-bit DMA transfers, not sure why why we have to subtract one
                          true);
}


void APA102::set_led(uint16_t index, APA102_LED led) {
    if(index < strip_len) {
        strip[index + 1] = led;
    }
}


void APA102::set_led(uint16_t index, uint8_t red, uint8_t green, uint8_t blue) {
    set_led(index, red, green, blue, max_brightness);
}


void APA102::set_led(uint16_t index, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness) {
    if(index < strip_len) {
        strip[index + 1].unused = 7;
        strip[index + 1].brightness = brightness;
        strip[index + 1].red = red;
        strip[index + 1].green = green;
        strip[index + 1].blue = blue;
    }
}


APA102::~APA102() {
    delete strip;
};
