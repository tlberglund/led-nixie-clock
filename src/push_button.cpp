

#include <cstdio>
#include "push_button.h"
#include "projdefs.h"
#include "pico/stdlib.h"
#include "pico/aon_timer.h"
#include "inttypes.h"



PushButton::PushButton(int pin, bool active_high, int pull_dir) {
      this->pin = pin;
      this->polarity = active_high;
      this->pullup_dir = pull_dir;
      this->state = STATE_IDLE;
      this->last_press_time = 0;
      this->last_release_time = 0;
      this->press_handler = NULL;
      this->long_press_handler = NULL;
      this->double_press_handler = NULL;
      this->double_release_handler = NULL;

      gpio_init(pin);
      gpio_set_dir(pin, GPIO_IN);
      if(pull_dir == BUTTON_PULL_UP) {
         printf("PULL UP\n");
         gpio_pull_up(pin);
      }
      else if(pull_dir == BUTTON_PULL_DOWN) {
         printf("PULL DOWN\n");
         gpio_pull_down(pin);
      }
      else {
         gpio_disable_pulls(pin);
      }

      printf("PushButton initialized on pin %d, active_high=%d, pull_dir=%d\n", pin, active_high, pull_dir);
}


bool PushButton::is_pressed() {
   return state == STATE_PRESSED || state == STATE_LONG_PRESS;
}


bool PushButton::is_released() {
   return state == STATE_IDLE;
}


void PushButton::process_tick() {

   bool button_down = read_input();
   uint64_t now = get_time();

   switch(state) {
      case STATE_IDLE:
         if(button_down) {
            state_transition(STATE_MAYBE_PRESSED);
            last_press_time = now;
         }
         break;

      case STATE_MAYBE_PRESSED:
         if(button_down) {
            if((now - last_press_time) > PRESS_TIME) {
               state_transition(STATE_PRESSED);
               if(press_handler) {
                  (*press_handler)(pin);
               }
            }
         } else {
            state_transition(STATE_IDLE);
         }
         break;

      case STATE_PRESSED:
         if(button_down == false) {
            state_transition(STATE_MAYBE_RELEASED);
            last_release_time = now;
         } 
         else {
            if((now - last_press_time) > LONG_PRESS_TIME) {
               state_transition(STATE_LONG_PRESS);
               if(long_press_handler) {
                  (*long_press_handler)(pin);
               }
            }
         }
         break;

      case STATE_LONG_PRESS:
         if(button_down == false) {
            state_transition(STATE_MAYBE_LONG_RELEASED);
            last_release_time = now;
         }
         break;

      case STATE_MAYBE_LONG_RELEASED:
         if(button_down) {
            state_transition(STATE_LONG_PRESS);
         }
         else {
            if((now - last_release_time) > RELEASE_TIME) {
               state_transition(STATE_IDLE);
            }
         }
         break;

      case STATE_MAYBE_RELEASED:
         if(button_down) {
            state_transition(STATE_PRESSED);
         }
         else {
            if((now - last_release_time) > RELEASE_TIME) {
               state_transition(STATE_MAYBE_IDLE);
            }
         } 
         break;

      case STATE_MAYBE_IDLE:
         if(button_down) {
            state_transition(STATE_MAYBE_DOUBLE_PRESS);
            last_press_time = now;
         } 
         else {
            if((now - last_release_time) > DOUBLE_PRESS_RELEASE_TIME) {
               state_transition(STATE_IDLE);
               if(release_handler) {
                  (*release_handler)(pin);
               }
            }
         }
         break;

      case STATE_MAYBE_DOUBLE_PRESS:
         if(button_down) {
            if((now - last_press_time) > PRESS_TIME) {
               state_transition(STATE_DOUBLE_PRESS);
               if(double_press_handler) {
                  (*double_press_handler)(pin);
               }
            }
         } 
         else {
            if((now - last_release_time) > RELEASE_TIME) {
               state_transition(STATE_IDLE);
            }
         }
         break;

      case STATE_DOUBLE_PRESS:
         if(button_down == false) {
            state_transition(STATE_MAYBE_DOUBLE_RELEASE);
            last_release_time = now;
         }
         break;

      case STATE_MAYBE_DOUBLE_RELEASE:
         if(button_down) {
            state_transition(STATE_DOUBLE_PRESS);
         } 
         else {
            if((now - last_release_time) > RELEASE_TIME) {
               state_transition(STATE_IDLE);
               if(double_release_handler) {
                  (*double_release_handler)(pin);
               }
            }
         }
         break;

      default:
         state_transition(STATE_IDLE);
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
