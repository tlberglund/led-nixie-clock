
#ifndef __PUSH_BUTTONS_H__
#define __PUSH_BUTTONS_H__

#include <cstdio>
#include <list>

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "push_button.h"




class PushButtons {
   public:
      void add_button(PushButton *push_button) { buttons.push_back(push_button); };

      static PushButtons& getInstance() {
         static PushButtons instance;

         return instance;
      }

      std::list<PushButton *> buttons;

   private:
      PushButtons();

      // BaseType_t button_task;
      // std::list<PushButton *> buttons;
};


#endif

// __PUSH_BUTTONS_H__
