#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

#define GPS_RX_PIN (16) 
#define GPS_TX_PIN (17)
#define GPS_UART_PORT (UART_NUM_2)
#define BUF_SIZE (1024)

static const char *TAG = "GPS_MONITOR";

void gps_read_task(void *arg) {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_driver_install(GPS_UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(GPS_UART_PORT, &uart_config);
    uart_set_pin(GPS_UART_PORT, GPS_TX_PIN, GPS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    while (1) {

        int len = uart_read_bytes(GPS_UART_PORT, data, BUF_SIZE - 1, 100 / portTICK_PERIOD_MS);
        
        if (len > 0) {
            data[len] = '\0';
            printf("%s", (char *)data); 
        }
    }
    free(data);
}

void app_main(void) {
    xTaskCreate(gps_read_task, "gps_read_task", 4096, NULL, 10, NULL);
}