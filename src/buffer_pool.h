#ifndef __BUFFER_POOL_H__
#define __BUFFER_POOL_H__

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <list>


class BufferPool {
   public:
      BufferPool(size_t pool_size, size_t buffer_size);
      ~BufferPool();
      
      void *allocate();
      void deallocate(void *buffer);

      size_t get_pool_size() const { return pool_size; }
      size_t get_buffer_size() const { return buffer_size; }
      size_t get_available_buffers() const { return buffers.size(); }

   private:
      std::list <void *>buffers;
      size_t pool_size;
      size_t buffer_size;

      SemaphoreHandle_t allocate_mutex;
      SemaphoreHandle_t counting_semaphore;

      // static void allocate_test_task__(void *pvParameters);
      // static void deallocate_test_task__(void *pvParameters);
      // void buffer_fuzz_test();
      // std::list <void *>test_buffers__;
};


#endif