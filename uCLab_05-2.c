#include <stdio.h>
#include <inttypes.h>
#include "driver/gpio.h"

// include FreeRTOS header for vTaskDelay and portTICK_PERIOD_MS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// กำหนดค่า GPIO (อ้างอิงจากเมนู CONFIG)
#define GPIO_INPUT_IO_0     CONFIG_GPIO_INPUT_0
#define GPIO_INPUT_IO_1     CONFIG_GPIO_INPUT_1
#define GPIO_INPUT_IO_2     CONFIG_GPIO_INPUT_2

#define GPIO_OUTPUT_IO_0    4  // ตัวอย่างขา output (คุณสามารถเปลี่ยนตามวงจร)
#define GPIO_OUTPUT_IO_1    5

#define GPIO_INPUT_PIN_POLL_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
#define GPIO_INPUT_PIN_INTR_SEL  (1ULL<<GPIO_INPUT_IO_2)

#define ESP_INTR_FLAG_DEFAULT 0
static QueueHandle_t gpio_evt_queue = NULL;

// ISR สำหรับ Interrupt
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// Task แสดงผลเมื่อเกิด Interrupt
static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%" PRIu32 "] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}

void app_main(void)
{
    gpio_config_t io_conf = {};

    // ******************** Polling Configuration
    // disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // bit mask of the pins (GPIO 0 & 1)
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_POLL_SEL;
    // disable pull-down / enable pull-up
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    // configuration to GPIO
    gpio_config(&io_conf);

    // ******************** Interrupt Configuration
    // enable interrupt on negative edge
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    // bit mask of the pin (GPIO 2)
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_INTR_SEL;
    // configuration to GPIO
    gpio_config(&io_conf);

    // set ISR for interrupt service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // Hook isr handler for specific GPIO Pin
    gpio_isr_handler_add(GPIO_INPUT_IO_2, gpio_isr_handler, (void*)GPIO_INPUT_IO_2);

    // ******************** Create a Task & Queue
    // create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    // create a display task for ISR
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    // ******************** Main Loop (Polling)
    int cnt = 0;
    for(;;){
        printf("cnt: %d\n", cnt++);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
        // ตัวอย่างการสั่ง Output (ถ้ามี)
        gpio_set_level(GPIO_OUTPUT_IO_0, cnt & 0x01);
        gpio_set_level(GPIO_OUTPUT_IO_1, cnt & 0x02);
        
        // Polling reading
        printf("GPIO[%d] poll, val: %d\n", GPIO_INPUT_IO_0, gpio_get_level(GPIO_INPUT_IO_0));
        printf("GPIO[%d] poll, val: %d\n", GPIO_INPUT_IO_1, gpio_get_level(GPIO_INPUT_IO_1));
    }
}