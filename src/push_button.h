
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


#define STATE_IDLE 0
#define STATE_MAYBE_PRESSED 1
#define STATE_PRESSED 2
#define STATE_LONG_PRESS 3
#define STATE_MAYBE_RELEASED 4
#define STATE_MAYBE_LONG_RELEASED 5
#define STATE_MAYBE_IDLE 6
#define STATE_MAYBE_DOUBLE_PRESS 7
#define STATE_DOUBLE_PRESS 8
#define STATE_MAYBE_DOUBLE_RELEASE 9

#define PRESS_TIME                  50 * 1000
#define LONG_PRESS_TIME            750 * 1000
#define RELEASE_TIME                50 * 1000
#define DOUBLE_PRESS_RELEASE_TIME  300 * 1000

const static char *state_names[] = {
   "IDLE",
   "MAYBE_PRESSED",
   "PRESSED",
   "LONG_PRESS",
   "MAYBE_RELEASED",
   "MAYBE_LONG_RELEASED",
   "MAYBE_IDLE",
   "MAYBE_DOUBLE_PRESS",
   "DOUBLE_PRESS",
   "MAYBE_DOUBLE_RELEASE"
};



static void debug_press_handler(int pin) {
   printf("DEBUG: PushButton %d PRESSED\n", pin);
}

static void debug_long_press_handler(int pin) {
   printf("DEBUG: PushButton %d LONG PRESSED\n", pin);
}

static void debug_release_handler(int pin) {
   printf("DEBUG: PushButton %d RELEASED\n", pin);
}

static void debug_double_press_handler(int pin) {
   printf("DEBUG: PushButton %d DOUBLE PRESSED\n", pin);
}

static void debug_double_release_handler(int pin) {
   printf("DEBUG: PushButton %d DOUBLE RELEASED\n", pin);
}

class PushButton {
   public:
      PushButton(int pin, bool active_high = true, int pull_dir = BUTTON_PULL_UP);

      void set_press_handler(void (*handler)(int pin)) { press_handler = handler;};
      void set_long_press_handler(void (*handler)(int pin)) { long_press_handler = handler; };
      void set_release_handler(void (*handler)(int pin)) { release_handler = handler; };
      void set_double_press_handler(void (*handler)(int pin)) { double_press_handler = handler; };
      void set_double_release_handler(void (*handler)(int pin)) { double_release_handler = handler; };

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
      void state_transition(int new_state) {
         // printf("PushButton %02d: %s->%s\n", pin, state_names[state], state_names[new_state]);
         state = new_state;
      }
};


#endif

// __PUSH_BUTTON_H__
