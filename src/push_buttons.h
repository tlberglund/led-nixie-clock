
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
      void add(PushButton *push_button) { buttons.push_back(push_button); };

      // Do not call during static initialization
      static PushButtons& getInstance() {
         static PushButtons instance;

         return instance;
      }

      void init();

      std::list<PushButton *> buttons;

   private:
      PushButtons();
};


#endif

// __PUSH_BUTTONS_H__
