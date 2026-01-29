#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define STACK_SIZE 2048
#define TASK_PRIORITY 10

const char *pcTextForTask1 = "Task 1 is running\n";
const char *pcTextForTask2 = "Task 2 is running\n";

void vTaskFunction(void *pvParameters) {
    while(1){
        printf("%s", (char *)pvParameters);
        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }
}

void app_main(void) {
    xTaskCreate( vTaskFunction, //Task function to process
        "Task1",                //Task name
        STACK_SIZE,             //Stack size
        (void *)pcTextForTask1, //Parameter to pass throug each task
        TASK_PRIORITY,          //Task priority
        NULL                    //Pointer to task handler
    );

    xTaskCreate( vTaskFunction, 
        "Task2",
        STACK_SIZE,
        (void *)pcTextForTask2, 
        TASK_PRIORITY, 
        NULL 
    );

    while (1)
    {
        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }
}