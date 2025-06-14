


#include "push_button.h"
#include "projdefs.h"
#include "pico/stdlib.h"
#include "pico/aon_timer.h"


#define STATE_IDLE 0
#define STATE_MAYBE_PRESSED 1
#define STATE_PRESSED 2
#define STATE_LONG_PRESS 4
#define STATE_MAYBE_RELEASED 5
#define STATE_MAYBE_IDLE 6
#define STATE_MAYBE_DOUBLE_PRESS 8
#define STATE_DOUBLE_PRESS 9
#define STATE_MAYBE_DOUBLE_RELEASE 10

#define PRESS_TIME 100      // T1
#define LONG_PRESS_TIME 750 // T2
#define RELEASE_TIME 100    // T3


PushButton::PushButton(int pin, bool active_high, int pull_dir) {
      this->pin = pin;
      this->polarity = active_high;
      this->pullup_dir = pull_dir;
      this->state = STATE_IDLE;
      this->last_press_time = 0;
      this->last_release_time = 0;
      this->press_handler = NULL;
      this->long_press_handler = NULL;
      this->release_handler = NULL;
      this->double_press_handler = NULL;
      this->double_release_handler = NULL;

      gpio_init(pin);
      gpio_set_dir(pin, GPIO_IN);
      if(pull_dir == BUTTON_PULL_UP) {
         gpio_pull_up(pin);
      }
      else if(pull_dir == BUTTON_PULL_DOWN) {
         gpio_pull_down(pin);
      }
      else {
         gpio_disable_pulls(pin);
      }
}


void PushButton::process_tick() {

   bool button_down = read_input();

   switch(state) {
      case STATE_IDLE:
         if(button_down) { 
            state = STATE_MAYBE_PRESSED;
            last_press_time = get_time();
         }
         break;

      case STATE_MAYBE_PRESSED:
         if(button_down) {
            if((get_time() - last_press_time) > PRESS_TIME) {
               state = STATE_PRESSED;
               if(press_handler) {
                  (*press_handler)(pin);
               }
            }
         } else {
            state = STATE_IDLE;
         }
         break;

      case STATE_PRESSED:
         if(button_down == false) {
            state = STATE_MAYBE_RELEASED;
            last_release_time = get_time();
         } 
         else {
            if((get_time() - last_press_time) > LONG_PRESS_TIME) {
               state = STATE_LONG_PRESS;
               if(long_press_handler) {
                  (*long_press_handler)(pin);
               }
            }
         }
         break;

      case STATE_LONG_PRESS:
         break;

      case STATE_MAYBE_RELEASED:
         if(button_down) {
            state = STATE_PRESSED;
         }
         else {
            if((get_time() - last_release_time) > RELEASE_TIME) {
               state = STATE_MAYBE_IDLE;
            }
         } 
         break;

      case STATE_MAYBE_IDLE:
         if(button_down) {
            state = STATE_MAYBE_DOUBLE_PRESS;
            last_press_time = get_time();
         } 
         else {
            if((get_time() - last_release_time) > RELEASE_TIME) {
               state = STATE_IDLE;
               if(release_handler) {
                  (*release_handler)(pin);
               }
            }
         }
         break;

      case STATE_MAYBE_DOUBLE_PRESS:
         if(button_down) {
            if((get_time() - last_press_time) > PRESS_TIME) {
               state = STATE_DOUBLE_PRESS;
               if(double_press_handler) {
                  (*double_press_handler)(pin);
               }
            }
         } 
         else {
            if((get_time() - last_release_time) > RELEASE_TIME) {
               state = STATE_IDLE;
            }
         }
         break;

      case STATE_DOUBLE_PRESS:
         if(button_down == false) {
            state = STATE_MAYBE_DOUBLE_RELEASE;
            last_release_time = get_time();
         }
         break;

      case STATE_MAYBE_DOUBLE_RELEASE:
         if(button_down) {
            state = STATE_DOUBLE_PRESS;
         } 
         else {
            if((get_time() - last_release_time) > RELEASE_TIME) {
               state = STATE_IDLE;
               if(double_release_handler) {
                  (*double_release_handler)(pin);
               }
            }
         }
         break;

      default:
         state = STATE_IDLE;
         break;
   }
}


uint64_t PushButton::get_time() {
   return time_us_64();
}


bool PushButton::read_input() {
   if(polarity) {
      return gpio_get(pin) == 1;
   } else {
      return gpio_get(pin) == 0;
   }
}
