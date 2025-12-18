#include <stdio.h>
#include <inttypes.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// กำหนดขาตามใบงาน: ปุ่มกด GPIO5, LED ดวงที่ 2 GPIO19 [cite: 9, 10]
#define BUTTON_GPIO   5
#define LED_2_GPIO    19

#define GPIO_INPUT_PIN_SEL  (1ULL << BUTTON_GPIO)
#define GPIO_OUTPUT_PIN_SEL (1ULL << LED_2_GPIO)
#define ESP_INTR_FLAG_DEFAULT 0

static QueueHandle_t gpio_evt_queue = NULL;
static int led2_state = 0; // ตัวแปรเก็บสถานะปัจจุบันของ LED 2

// ISR สำหรับส่งค่าเมื่อมีการกดปุ่ม [cite: 97, 98, 99]
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// Task สำหรับรับค่าจาก Queue และทำการ Toggle LED [cite: 101, 104, 105]
static void toggle_led_task(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            led2_state = !led2_state; // เปลี่ยนสถานะ (Toggle) 
            gpio_set_level(LED_2_GPIO, led2_state);
            printf("Button GPIO[%" PRIu32 "] pressed! LED 2 is now: %d\n", io_num, led2_state);
        }
    }
}

void app_main(void)
{
    gpio_config_t io_conf = {};

    // 1. ตั้งค่า LED 2 เป็น Output [cite: 29]
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    // 2. ตั้งค่าปุ่มกดเป็น Input แบบ Interrupt (ตรวจจับการกดปุ่มลง Ground) [cite: 116, 118, 122]
    io_conf.intr_type = GPIO_INTR_NEGEDGE; // ตรวจจับขอบขาลง (กดปุ่ม)
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.pull_up_en = 1; // เปิด Pull-up ภายในบอร์ด [cite: 122]
    gpio_config(&io_conf);

    // 3. เริ่มระบบ Interrupt และ Task [cite: 126, 128, 131, 133]
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(toggle_led_task, "toggle_led_task", 2048, NULL, 10, NULL);
    
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler, (void*)BUTTON_GPIO);

    while(1) {
        vTaskDelay(portMAX_DELAY); // ให้ระบบทำงานผ่าน Task และ ISR
    }
}