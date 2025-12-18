#include <stdio.h>
#include "driver/gpio.h"

// include FreeRTOS header for vTaskDelay and portTICK_PERIOD_MS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// กำหนดหมายเลข GPIO ตามที่ตั้งค่าใน Menuconfig หรือกำหนดเอง
// ในที่นี้สมมติว่าเป็น Pin 18 และ 19 ตาม Comment ในรูป
#define GPIO_OUTPUT_IO_0    18 
#define GPIO_OUTPUT_IO_1    19
#define GPIO_OUTPUT_PIN_SET ((1ULL << GPIO_OUTPUT_IO_0) | (1ULL << GPIO_OUTPUT_IO_1))

void app_main(void)
{
    // สร้าง struct สำหรับการตั้งค่า GPIO
    gpio_config_t io_conf = {};

    // disable interrupt (ปิดการขัดจังหวะ)
    io_conf.intr_type = GPIO_INTR_DISABLE;

    // set as a output mode (ตั้งค่าเป็นโหมด Output)
    io_conf.mode = GPIO_MODE_OUTPUT;

    // bit mask of the pins that you want to set, e.g. GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SET;

    // disable pull-down mode (ปิดตัวต้านทาน Pull-down)
    io_conf.pull_down_en = 0;

    // disable pull-up mode (ปิดตัวต้านทาน Pull-up)
    io_conf.pull_up_en = 0;

    // configuration to GPIO (นำค่าที่ตั้งไว้ไปใช้งาน)
    gpio_config(&io_conf);

    int cnt = 0;
    for(;;) {
        printf("cnt: %d\n", cnt++);
        
        // หน่วงเวลา 1 วินาที
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // สลับสถานะ Logic ของ Pin ตามค่าของตัวแปร cnt
        // ใช้ Bitwise AND (&) เพื่อดึงค่า Bit ที่ต้องการมาแสดงผล
        gpio_set_level(GPIO_OUTPUT_IO_0, cnt & 0x01);
        gpio_set_level(GPIO_OUTPUT_IO_1, (cnt & 0x02) >> 1); // เลื่อนบิตมาที่ตำแหน่ง 0 เพื่อให้เป็น 0 หรือ 1
    }
}