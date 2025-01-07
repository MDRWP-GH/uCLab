#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/mcpwm_prelude.h"

const static char *TAG = "ADC and MCPWM";

#define ADC1_CHANNEL             ADC_CHANNEL_4  // ADC Channel สำหรับตัวต้านทานปรับค่าได้
#define ADC_ATTEN                ADC_ATTEN_DB_11
#define SERVO_MIN_PULSEWIDTH_US  500   // ความกว้างของพัลส์ต่ำสุด (ไมโครวินาที)
#define SERVO_MAX_PULSEWIDTH_US  2500  // ความกว้างของพัลส์สูงสุด (ไมโครวินาที)
#define SERVO_MIN_DEGREE         -90   // มุมต่ำสุด
#define SERVO_MAX_DEGREE         90    // มุมสูงสุด
#define SERVO_PULSE_GPIO         13    // GPIO ที่เชื่อมต่อกับสัญญาณ PWM

static inline uint32_t example_angle_to_compare(int angle)
{
    return (angle - SERVO_MIN_DEGREE) * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) / (SERVO_MAX_DEGREE - SERVO_MIN_DEGREE) + SERVO_MIN_PULSEWIDTH_US;
}

void app_main(void)
{
    //------------ADC1 Init-------------//
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    //------------ADC1 Config-------------//
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_CHANNEL, &config));

    //------------MCPWM Init-------------//
    mcpwm_timer_handle_t timer = NULL;
    mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000, // 1MHz, 1us ต่อ tick
        .period_ticks = 20000,    // 20000 ticks, 20ms
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

    mcpwm_oper_handle_t oper = NULL;
    mcpwm_operator_config_t operator_config = {
        .group_id = 0,
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    mcpwm_cmpr_handle_t comparator = NULL;
    mcpwm_comparator_config_t comparator_config = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator));

    mcpwm_gen_handle_t generator = NULL;
    mcpwm_generator_config_t generator_config = {
        .gen_gpio_num = SERVO_PULSE_GPIO,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator));

    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_angle_to_compare(0)));

    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator,
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW)));

    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));

    while (1)
    {
        int adc_value;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC1_CHANNEL, &adc_value));
        ESP_LOGI(TAG, "ADC Raw Data: %d", adc_value);

        int angle = (adc_value * (SERVO_MAX_DEGREE - SERVO_MIN_DEGREE) / 4095) + SERVO_MIN_DEGREE;
        ESP_LOGI(TAG, "Servo Angle: %d", angle);
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, example_angle_to_compare(angle)));

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
}
