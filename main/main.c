/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "ssd1306.h"
#include "gfx.h"

#include "pico/stdlib.h"
#include <stdio.h>

const uint BTN_1_OLED = 28;
const uint BTN_2_OLED = 26;
const uint BTN_3_OLED = 27;

const uint LED_1_OLED = 20;
const uint LED_2_OLED = 21;
const uint LED_3_OLED = 22;


int PIN_TRIGER = 4;
int PIN_ECHO = 5;

QueueHandle_t xQueueTime;
QueueHandle_t xQueueDistance;
SemaphoreHandle_t xSemaphoreTrigger;

void oled1_btn_led_init(void) {
    gpio_init(LED_1_OLED);
    gpio_set_dir(LED_1_OLED, GPIO_OUT);

    gpio_init(LED_2_OLED);
    gpio_set_dir(LED_2_OLED, GPIO_OUT);

    gpio_init(LED_3_OLED);
    gpio_set_dir(LED_3_OLED, GPIO_OUT);

    gpio_init(BTN_1_OLED);
    gpio_set_dir(BTN_1_OLED, GPIO_IN);
    gpio_pull_up(BTN_1_OLED);

    gpio_init(BTN_2_OLED);
    gpio_set_dir(BTN_2_OLED, GPIO_IN);
    gpio_pull_up(BTN_2_OLED);

    gpio_init(BTN_3_OLED);
    gpio_set_dir(BTN_3_OLED, GPIO_IN);
    gpio_pull_up(BTN_3_OLED);
}

void pin_callback(uint gpio, uint32_t events){
    static uint32_t start_echo, end_echo, delta;
    if (events == GPIO_IRQ_EDGE_RISE) { // rise edge
            start_echo = to_us_since_boot(get_absolute_time());
        }
    else if (events == GPIO_IRQ_EDGE_FALL) { // fall 
            end_echo = to_us_since_boot(get_absolute_time());
            delta = end_echo - start_echo;
            xQueueSendFromISR(xQueueTime, &delta, NULL);
    }
}

void trigger_task(){
    gpio_init(PIN_TRIGER);
    gpio_set_dir(PIN_TRIGER, GPIO_OUT);

    while(true){
        gpio_put(PIN_TRIGER, 1);
        sleep_ms(5);
        gpio_put(PIN_TRIGER, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
        xSemaphoreGive(xSemaphoreTrigger);
    }
}

void echo_task(){
    gpio_init(PIN_ECHO);
    gpio_set_dir(PIN_ECHO, GPIO_IN);

    uint32_t distancia, delta;
    while(true){
        if(xQueueReceive(xQueueTime, &delta, portMAX_DELAY) == pdTRUE){
            distancia = delta * 0.017015;
            printf("%u", distancia);
            xQueueSend(xQueueDistance, &distancia, portMAX_DELAY);
        }
    }
}

void oled_task(){
    printf("Inicializando Driver\n");
    ssd1306_init();

    printf("Inicializando GLX\n");
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);

    printf("Inicializando btn and LEDs\n");
    oled1_btn_led_init();
    uint32_t distancia;
    int loading;
    char distancia_str[12];

    while(true){
        if(xSemaphoreTake(xSemaphoreTrigger, portMAX_DELAY) == pdTRUE){
            if(xQueueReceive(xQueueDistance, &distancia, pdMS_TO_TICKS(1000)) == pdTRUE){
                sprintf(distancia_str, "%u cm", distancia);
                gfx_clear_buffer(&disp);
                gfx_draw_string(&disp, 0, 0, 1, "DISTANCIA: ");
                gfx_draw_string(&disp, 0, 0, 1, distancia_str);
                if(distancia <= 128){
                    gfx_draw_line(&disp, 0, 20, distancia*1.3, 20);
                }
                else{
                    gfx_draw_line(&disp, 0, 20, 128, 20);
                }
                gfx_show(&disp);   
            }
        }
        else{
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "ERRO");
            gfx_show(&disp);
        }
    }
}


int main() {
    stdio_init_all();
    gpio_set_irq_enabled_with_callback(PIN_ECHO, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &pin_callback);

    xQueueTime = xQueueCreate(32, sizeof(int));
    xQueueDistance = xQueueCreate(32, sizeof(int));
    xSemaphoreTrigger = xSemaphoreCreateBinary();

    xTaskCreate(trigger_task, "Trigger", 256, NULL, 1, NULL);
    xTaskCreate(echo_task, "Echo", 256, NULL, 1, NULL);
    xTaskCreate(oled_task, "Oled", 2048, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}


// void oled1_demo_1(void *p) {
//     printf("Inicializando Driver\n");
//     ssd1306_init();

//     printf("Inicializando GLX\n");
//     ssd1306_t disp;
//     gfx_init(&disp, 128, 32);

//     printf("Inicializando btn and LEDs\n");
//     oled1_btn_led_init();

//     char cnt = 15;
//     while (1) {

//         if (gpio_get(BTN_1_OLED) == 0) {
//             cnt = 15;
//             gpio_put(LED_1_OLED, 0);
//             gfx_clear_buffer(&disp);
//             gfx_draw_string(&disp, 0, 0, 1, "LED 1 - ON");
//             gfx_show(&disp);
//         } else if (gpio_get(BTN_2_OLED) == 0) {
//             cnt = 15;
//             gpio_put(LED_2_OLED, 0);
//             gfx_clear_buffer(&disp);
//             gfx_draw_string(&disp, 0, 0, 1, "LED 2 - ON");
//             gfx_show(&disp);
//         } else if (gpio_get(BTN_3_OLED) == 0) {
//             cnt = 15;
//             gpio_put(LED_3_OLED, 0);
//             gfx_clear_buffer(&disp);
//             gfx_draw_string(&disp, 0, 0, 1, "LED 3 - ON");
//             gfx_show(&disp);
//         } else {

//             gpio_put(LED_1_OLED, 1);
//             gpio_put(LED_2_OLED, 1);
//             gpio_put(LED_3_OLED, 1);
//             gfx_clear_buffer(&disp);
//             gfx_draw_string(&disp, 0, 0, 1, "PRESSIONE ALGUM");
//             gfx_draw_string(&disp, 0, 10, 1, "BOTAO");
//             gfx_draw_line(&disp, 15, 27, cnt,
//                           27);
//             vTaskDelay(pdMS_TO_TICKS(50));
//             if (++cnt == 112)
//                 cnt = 15;

//             gfx_show(&disp);
//         }
//     }
// }

// void oled1_demo_2(void *p) {
//     printf("Inicializando Driver\n");
//     ssd1306_init();

//     printf("Inicializando GLX\n");
//     ssd1306_t disp;
//     gfx_init(&disp, 128, 32);

//     printf("Inicializando btn and LEDs\n");
//     oled1_btn_led_init();

//     while (1) {

//         gfx_clear_buffer(&disp);
//         gfx_draw_string(&disp, 0, 0, 1, "Mandioca");
//         gfx_show(&disp);
//         vTaskDelay(pdMS_TO_TICKS(150));

//         gfx_clear_buffer(&disp);
//         gfx_draw_string(&disp, 0, 0, 2, "Batata");
//         gfx_show(&disp);
//         vTaskDelay(pdMS_TO_TICKS(150));

//         gfx_clear_buffer(&disp);
//         gfx_draw_string(&disp, 0, 0, 4, "Inhame");
//         gfx_show(&disp);
//         vTaskDelay(pdMS_TO_TICKS(150));
//     }
// }