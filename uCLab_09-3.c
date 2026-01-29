#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

SemaphoreHandle_t xMutex = NULL;
const TickType_t xMaxBlockTimeTicks = 0x20;

static void vPrintString(const char *pcString) {
    xSemaphoreTake(xMutex, portMAX_DELAY);

        printf("%s", pcString);
        fflush(stdout);

        xSemaphoreGive(xMutex);
}

static void prvPrintTask(void *pvParameters) {
    char *pcString = (char *)pvParameters;
    const TickType_t xSlowDown = pdMS_TO_TICKS( 5UL );

    while (1) {
        vPrintString(pcString);
        vTaskDelay( rand() % xMaxBlockTimeTicks );
        vTaskDelay( xSlowDown );
    }
}

const char *pvStringforTask1 = "Task 1 *******************************************************\r\n";
const char *pvStringforTask2 = "Task 2 -------------------------------------------------------\r\n";

void app_main(void) {
    xMutex = xSemaphoreCreateMutex();

        xTaskCreate(prvPrintTask, "Task 1", 2048, (void *)pvStringforTask1, 1, NULL);
        xTaskCreate(prvPrintTask, "Task 2", 2048, (void *)pvStringforTask2, 2, NULL);

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}