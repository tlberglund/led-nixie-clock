
#include "clock.h"


Clock::Clock() : leds_hours_10(DIGIT_ROW_WIDTH * 3, 12, 13, pio0, 0),
                 leds_hours_1(DIGIT_ROW_WIDTH * 10, 10, 11, pio0, 1),
                 leds_minutes_10(DIGIT_ROW_WIDTH * 6, 8, 9, pio0, 2),
                 leds_minutes_1(DIGIT_ROW_WIDTH * 10, 6, 7, pio0, 3),
                 leds_seconds_10(DIGIT_ROW_WIDTH * 6, 4, 5, pio1, 0),
                 leds_seconds_1(DIGIT_ROW_WIDTH * 10, 2, 3, pio1, 1) {
    leds_hours_10.clear_strip();
    leds_hours_1.clear_strip();
    leds_minutes_10.clear_strip();
    leds_minutes_1.clear_strip();
    leds_seconds_10.clear_strip();
    leds_seconds_1.clear_strip();
}


APA102 *Clock::get_strip(clock_digit_t digit) {
    switch(digit) {
        case HOURS_10:
            return &leds_hours_10;
        case HOURS_1:
            return &leds_hours_1;
        case MINUTES_10:
            return &leds_minutes_10;
        case MINUTES_1:
            return &leds_minutes_1;
        case SECONDS_10:
            return &leds_seconds_10;
        case SECONDS_1:
            return &leds_seconds_1;
    }
    return NULL;
}


void Clock::set_digit(clock_digit_t digit, uint8_t value) {
    APA102 *strip = get_strip(digit);
    strip->clear_strip();
    for(uint8_t led = 0; led < DIGIT_ROW_WIDTH; led++) {
        strip->set_led(led + (value * DIGIT_ROW_WIDTH),
                       digit_pattern[(digit * DIGIT_ROW_WIDTH) + led].red,
                       digit_pattern[(digit * DIGIT_ROW_WIDTH) + led].green,
                       digit_pattern[(digit * DIGIT_ROW_WIDTH) + led].blue,
                       digit_pattern[(digit * DIGIT_ROW_WIDTH) + led].brightness);
    }
    strip->update_strip();
}


void Clock::set_digit_pattern(clock_digit_t digit, APA102_LED *pattern) {
    for(int led = 0; led < DIGIT_ROW_WIDTH; led++) {
        digit_pattern[(digit * DIGIT_ROW_WIDTH) + led] = pattern[led];
    }
}


void Clock::set_whole_pattern(APA102_LED *pattern) {
    for(int n = 0; n < DIGIT_LED_COUNT * NUM_DIGITS; n++) {
        digit_pattern[n] = pattern[n];
    }
}


void Clock::set_uniform_pattern(APA102_LED *pattern) {
    for(int digit = 0; digit < NUM_DIGITS; digit++) {
        for(int led = 0; led < DIGIT_ROW_WIDTH; led++) {
            digit_pattern[(digit * DIGIT_ROW_WIDTH) + led] = pattern[led];
        }
    }
}


void Clock::lamp_test(clock_digit_t digit) {
    APA102 *strip = get_strip(digit);
    strip->clear_strip();
    for(uint16_t n = 0; n < strip->get_strip_len(); n++) {
        strip->set_led(n, 255, 255, 255, 31);
    }

    strip->update_strip();
}


void Clock::lamp_test() {
    lamp_test(HOURS_10);
    lamp_test(HOURS_1);
    lamp_test(MINUTES_10);
    lamp_test(MINUTES_1);
    lamp_test(SECONDS_10);
    lamp_test(SECONDS_1);
}


void Clock::update_digits() {
    leds_hours_10.update_strip();
    leds_hours_1.update_strip();
    leds_minutes_10.update_strip();
    leds_minutes_1.update_strip();
    leds_seconds_10.update_strip();
    leds_seconds_1.update_strip();
}


void Clock::set_time(struct tm *time) {
    uint8_t hours_10, hours_1, minutes_10, minutes_1, seconds_10, seconds_1;

    hours_10 = time->tm_hour / 10;
    hours_1 = time->tm_hour % 10;
    minutes_10 = time->tm_min / 10;
    minutes_1 = time->tm_min % 10;
    seconds_10 = time->tm_sec / 10;
    seconds_1 = time->tm_sec % 10;

    set_digit(HOURS_10, hours_10);
    set_digit(HOURS_1, hours_1);
    set_digit(MINUTES_10, minutes_10);
    set_digit(MINUTES_1, minutes_1);
    set_digit(SECONDS_10, seconds_10);
    set_digit(SECONDS_1, seconds_1);
}