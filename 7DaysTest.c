#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_PIN GPIO_NUM_13 // เปลี่ยน GPIO ให้ตรงกับที่คุณใช้

static const char *TAG = "GPTimer";

typedef struct
{
    uint64_t event_count;
} example_queue_element_t;

static bool IRAM_ATTR example_timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
    QueueHandle_t queue = (QueueHandle_t)user_data; // Retrieve count value and send to queue
    example_queue_element_t ele = {
        .event_count = edata->count_value
    };
    xQueueSendFromISR(queue, &ele, &high_task_awoken);
    return (high_task_awoken == pdTRUE);
}

void app_main(void)
{
    example_queue_element_t ele;
    QueueHandle_t queue = xQueueCreate(10, sizeof(example_queue_element_t));
    if (!queue)
    {
        ESP_LOGE(TAG, "Creating queue failed");
        return;
    }

    gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    ESP_LOGI(TAG, "Create timer handle");
    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1 MHz, 1 tick = 1 µs
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = example_timer_on_alarm_cb,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, queue));

    ESP_LOGI(TAG, "Enable timer");
    ESP_ERROR_CHECK(gptimer_enable(gptimer));

    ESP_LOGI(TAG, "Start timer, auto-reload at alarm event");
    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = 250000, // period = 250ms
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
    ESP_ERROR_CHECK(gptimer_start(gptimer));

    int record = 40; // ตั้งค่าจำนวนครั้งที่ต้องการให้ไฟ LED กระพริบ (10 วินาที)
    while (record)
    {
        if (xQueueReceive(queue, &ele, pdMS_TO_TICKS(250)))
        {
            ESP_LOGI(TAG, "Timer reloaded, count=%llu", ele.event_count);
            gpio_set_level(LED_PIN, ele.event_count % 2); // สลับสถานะ LED
            record--;
        }
        else
        {
            ESP_LOGW(TAG, "Missed one count event");
        }
    }

    ESP_LOGI(TAG, "Stop timer");
    ESP_ERROR_CHECK(gptimer_stop(gptimer));

    ESP_LOGI(TAG, "Disable timer");
    ESP_ERROR_CHECK(gptimer_disable(gptimer));

    ESP_LOGI(TAG, "Delete timer");
    ESP_ERROR_CHECK(gptimer_del_timer(gptimer));

    vQueueDelete(queue);
}

//1 เราใช้ GPTimer เพื่อสร้างการหน่วงเวลา 250 มิลลิวินาทีและใช้ callback function เพื่อสลับสถานะของ LED ในทุกๆการเรียก callback

//2 เราใช้ gpio_set_level เพื่อตั้งค่าระดับของ LED ให้เป็น on (1) หรือ off (0)

//3 การกำหนดค่าของ GPTimer ใช้ความถี่ 1 MHz (1 tick = 1 µs) และตั้งค่าให้เกิด alarm ทุกๆ 250,000 ticks ซึ่งเท่ากับ 250 มิลลิวินาที

//4 xQueueSendFromISR และ xQueueReceive ใช้สำหรับส่งและรับข้อมูลระหว่าง ISR และ task หลัก

//5 LED จะสลับสถานะในทุกๆ 250 มิลลิวินาที ดังนั้น LED จะติด 250 มิลลิวินาทีและดับ 250 มิลลิวินาที