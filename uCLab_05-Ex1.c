#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// กำหนดขา LED ดวงที่ 1
#define LED_1_GPIO 18

void app_main(void) {
    // 1. ตั้งค่า GPIO สำหรับ LED
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,    // ไม่ใช้ Interrupt [cite: 27]
        .mode = GPIO_MODE_OUTPUT,          // ตั้งเป็น Output [cite: 29]
        .pin_bit_mask = (1ULL << LED_1_GPIO), // เลือกขา GPIO18 [cite: 31]
        .pull_down_en = 0,                 // ปิด Pull-down [cite: 33]
        .pull_up_en = 0                    // ปิด Pull-up [cite: 34]
    };
    gpio_config(&io_conf); [cite: 35]

    int state = 0;
    while(1) {
        // 2. สลับสถานะ LED (Toggle)
        state = !state;
        gpio_set_level(LED_1_GPIO, state); [cite: 41, 50]
        
        // 3. แสดงสถานะผ่าน Serial Monitor
        printf("LED 1 is %s\n", state ? "ON" : "OFF"); [cite: 39]
        
        // 4. หน่วงเวลา 1 วินาที (1000ms) [cite: 40]
        vTaskDelay(1000 / portTICK_PERIOD_MS); 
    }
}