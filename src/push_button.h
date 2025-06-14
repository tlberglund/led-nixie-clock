
#ifndef __PUSH_BUTTON_H__
#define __PUSH_BUTTON_H__

#include "hardware/gpio.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"


// Only true for the QPFN-80 package, not on the Pico boards, but hey, we have RAM
#define BUTTON_PULL_UP      1
#define BUTTON_PULL_DOWN    2
#define BUTTON_PULL_NEITHER 3


class PushButton {
   public:
      PushButton(int pin, bool active_high = true, int pull_dir = BUTTON_PULL_UP);

      void set_press_handler(int pin, void (*handler)(int pin)) { press_handler = handler;};
      void set_long_press_handler(int pin, void (*handler)(int pin)) { long_press_handler = handler; };
      void set_release_handler(int pin, void (*handler)(int pin)) { release_handler = handler; };
      void set_double_press_handler(int pin, void (*handler)(int pin)) { double_press_handler = handler; };
      void set_double_release_handler(int pin, void (*handler)(int pin)) { double_release_handler = handler; };

      void process_tick();

   private:
      int state;
      bool polarity; // true = active high
      int pin;
      uint8_t pullup_dir;
      uint64_t last_press_time;
      uint64_t last_release_time;

      void (*press_handler)(int pin);
      void (*long_press_handler)(int pin);
      void (*release_handler)(int pin);
      void (*double_press_handler)(int pin);
      void (*double_release_handler)(int pin);

      uint64_t get_time();
      bool read_input();
};


#endif

// __PUSH_BUTTON_H__
