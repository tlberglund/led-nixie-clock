
#ifndef __CLOCK_H__
#define __CLOCK_H__

#include "apa102.h"
#include <time.h>

#define DIGIT_LED_COUNT 50
#define DIGIT_ROW_WIDTH 5
#define NUM_DIGITS 6

typedef enum {
    HOURS_10 = 0,
    HOURS_1 = 1,
    MINUTES_10 = 2,
    MINUTES_1 = 3,
    SECONDS_10 = 4,
    SECONDS_1 = 5
} clock_digit_t;


class Clock {
    public:
        Clock();
        ~Clock();

        void set_digit(clock_digit_t digit, uint8_t value);

        // Set the DIGIT_ROW_WIDTH-wide pattern for a single digit
        void set_digit_pattern(clock_digit_t digit, APA102_LED *pattern);

        // Set the light pattern for the whole clock, from HOURS_10 to SECONDS_1
        void set_whole_pattern(APA102_LED *pattern);

        // Set the light pattern for the whole clock, with all digits the same
        void set_uniform_pattern(APA102_LED *pattern);

        void lamp_test(clock_digit_t digit);
        void lamp_test();

        void set_time(struct tm *time);

        void update_digits();

    private:
        APA102 leds_hours_10;
        APA102 leds_hours_1;
        APA102 leds_minutes_10;
        APA102 leds_minutes_1;
        APA102 leds_seconds_10;
        APA102 leds_seconds_1;

        APA102 *get_strip(clock_digit_t digit);

        APA102_LED digit_pattern[DIGIT_ROW_WIDTH * NUM_DIGITS];
};

#endif
