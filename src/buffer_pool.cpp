
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "buffer_pool.h"
#include "hardware/timer.h"


BufferPool::BufferPool(size_t pool_size, size_t buffer_size) : pool_size(pool_size), buffer_size(buffer_size)
{
   printf("BufferPool::BufferPool() - pool_size=%zu, buffer_size=%zu\n", pool_size, buffer_size);
   for(size_t i = 0; i < pool_size; i++)
   {
      char *buffer = new char[buffer_size];
      buffers.push_back(buffer);
   }

   allocate_mutex = xSemaphoreCreateMutex();
   counting_semaphore = xSemaphoreCreateCounting(pool_size, 0);

   printf("BufferPool::BufferPool() - %d buffers, mutex=%d, sem=%d\n", buffers.size(), allocate_mutex, counting_semaphore);
}


BufferPool::~BufferPool()
{
   for(auto buffer : buffers) {
      delete[] static_cast<char *>(buffer);
   }
   buffers.clear();

   vSemaphoreDelete(allocate_mutex);
   vSemaphoreDelete(counting_semaphore);
}


void *BufferPool::allocate()
{
   // printf("BufferPool::allocate() - about to take counting semaphore\n");
   // xSemaphoreTake(counting_semaphore, portMAX_DELAY);
   printf("BufferPool::allocate() - about to take allocate mutex\n");
   xSemaphoreTake(allocate_mutex, portMAX_DELAY);

   void *buffer = nullptr;
   if(!buffers.empty()) {
      buffer = buffers.front();
      buffers.pop_front();
   }
   else {
      // In case something went really quite pear-shaped
      printf("BufferPool::allocate() - no buffers available\n");
      xSemaphoreGive(counting_semaphore);
   }

   printf("BufferPool::allocate() - about to give allocate mutex\n");
   xSemaphoreGive(allocate_mutex);
   printf("BufferPool::allocate() - returning %p\n", buffer);
   return buffer;
}


void BufferPool::deallocate(void *buffer) {
   printf("BufferPool::deallocate() - freeing %p\n", buffer);
   if(buffer != nullptr) {
      printf("BufferPool::deallocate() - taking allocate mutex\n");
      xSemaphoreTake(allocate_mutex, portMAX_DELAY);
      buffers.push_back(buffer);
      xSemaphoreGive(allocate_mutex);
      printf("BufferPool::deallocate() - gave allocate mutex\n");
      // xSemaphoreGive(counting_semaphore);
   }
}

// void BufferPool::allocate_test_task__(void *pvParameters) {
//    srand(time_us_64());
//    BufferPool *pool = static_cast<BufferPool *>(pvParameters);
//    for(;;) {
//       int allocate_count = rand() % 20 + 1;
//       int delay = rand() % 100;

//       for(int i = 0; i < allocate_count; i++) {
//          void *buffer = pool->allocate();
//          pool->test_buffers__.push_front(buffer);
//          printf("ALLOCATED %p\n", buffer);
//       }

//       printf("ALLOCATE DELAY %03d\n", delay);
//       vTaskDelay(pdMS_TO_TICKS(delay));
//    }
// }

// void BufferPool::deallocate_test_task__(void *pvParameters) {
//    srand(time_us_64());
//    BufferPool *pool = static_cast<BufferPool *>(pvParameters);
//    for(;;) {
//       int deallocate_count = rand() % 20 + 1;
//       int delay = rand() % 100;

//       printf("DEALLOCATE DELAY %03d\n", delay);
//       vTaskDelay(pdMS_TO_TICKS(delay));

//       for(int i = 0; i < deallocate_count && !pool->test_buffers__.empty(); i++) {
//          void *buffer = pool->test_buffers__.front();
//          pool->test_buffers__.pop_front();
//          pool->deallocate(buffer);
//          printf("DEALLOCATED %p\n", buffer);
//       }
//    }
// }


// void BufferPool::buffer_fuzz_test() {

// }