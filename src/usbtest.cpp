#include <cstdio>
#include "pico/stdlib.h"
#include "pico/stdio.h"

int main()
{
   stdio_init_all(); // This must be called early

   // Optional: wait for USB connection
   while (!stdio_usb_connected())
   {
      sleep_ms(100);
   }

   int iteration = 0;
   for (;;)
   {
      printf("Hello %d!\n", iteration++);
      sleep_ms(100);
   }
}
