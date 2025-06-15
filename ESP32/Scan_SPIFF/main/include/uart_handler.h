#ifndef UART_HANDLER_H
#define UART_HANDLER_H

#include <driver/uart.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_system.h"
#include "esp_log.h"

// UART Configuration
#define UART_PORT_NUM   UART_NUM_1
#define UART_BAUD_RATE  115200
#define UART_RX_PIN     4
#define UART_TX_PIN     5
#define UART_BUF_SIZE   1024
#define MAX_PAYLOAD     12

#define TIMEZONE        "UTC+7" // Adjust as needed

extern QueueHandle_t xSensorQueue;

void uart_init(void);
void uart_parser_task(void *pvParameters);
void print_hex(const uint8_t *data, size_t len);
void firebase_sender_task(void *pvParameters);

#endif /* UART_HANDLER_H */
