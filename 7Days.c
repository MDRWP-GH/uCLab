#include <stdio.h>
#include "freetos/FreeRTOS.h"
#include "freetos/task.h"
#include "driver/gptimer.h"
#include "esp_log.h"

static const char *TAG = "GPTimer";

typedef struct
{
    uint64_t event_count;
} example_queue_element_t;

//โค้ดในส่วนของการ hook ISR ของตัว Timer 
static bool IRAM_ATTR example_timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
    QueueHandle_t queue = (QueueHandle_t)user_data; // Retrieve count value and send to queue
    example_queue_element_t ele = {
        .event_count = edata->count_value
    };
    xQueueSendFromISR(queue, &ele, &high_task_awoken);
    // Return whether we need to yield at the end of ISR
    return (high_task_awoken == pdTRUE);
}

//ส่วนต้นของการกำหนดค่าพารามิเตอร์ที่ต้องการ configure ให้กับตัว Timer เพื่อทำการนับเวลาจากสัญญาณนาฬิกา 
void app_main(void)
{
    example_queue_element_t ele;
    QueueHandle_t queue = xQueueCreate(10, sizeof(example_queue_element_t));
    if (!queue) {
        ESP_LOGE(TAG, "Creating queue failed");
        return;
    }

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

//โค้ดส่วนหลังเพื่อทำการ configure alarm event ให้กับตัว Timer โดยในที่นี้จะเกิด Interrupt ทุกๆ 1 วินาที และจะทำการโหลดค่าเริ่มต้นใหม่เพื่อวนนับไปจนกว่าจะสั่ง Timer หยุดการทำงาน 

    ESP_LOGI(TAG, "Start timer, auto-reload at alarm event");
    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = 1000000, // period = 1s
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
    ESP_ERROR_CHECK(gptimer_start(gptimer));

    int record = 4;
    while (record) {
        if (xQueueReceive(queue, &ele, pdMS_TO_TICKS(2000))) {
            ESP_LOGI(TAG, "Timer reloaded, count=%llu", ele.event_count);
            record--;
        } else {
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