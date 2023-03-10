/**
 * This sample application demonstrates how to detect a press of the
 * ESP-EYE board side button.
 */

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"

// Detection LED is controlled by GPIO 21 (check the schematic).
#define GPIO_DETECT_LED GPIO_NUM_21

// Side button is connected to GPIO15 (check the schematic).
#define GPIO_SIDE_BUTTON GPIO_NUM_15
// Wait period between two successive checks of the button press
// flag, in ms.
#define CHECK_PERIOD_MS 100

static const char TAG[] = "BTN and LED";

static volatile bool press_detected;

// GPIO interrupt handler.
static void gpio_interrupt_handler(void *arg) {

    press_detected = true;

}

void app_main(void) {

	esp_err_t rs;

	// Select GPIO function for the pin, enable pull-up, disable I/O.
	// No need to check returned status: it is always ESP_OK.
	gpio_reset_pin(GPIO_DETECT_LED);
	// Disable pull-up, as there is an external pull-down.
	rs = gpio_set_pull_mode(GPIO_DETECT_LED, GPIO_FLOATING);
	if (rs != ESP_OK) {
		ESP_LOGE(TAG, "Error from gpio_set_pull_mode: %s", esp_err_to_name(rs));
		goto exit_on_fatal_error;
	}
	rs = gpio_set_direction(GPIO_DETECT_LED, GPIO_MODE_OUTPUT);
	if (rs != ESP_OK) {
		ESP_LOGE(TAG, "Error from gpio_set_direction: %s", esp_err_to_name(rs));
		goto exit_on_fatal_error;
	}
	uint32_t led_level = 0;
	rs = gpio_set_level(GPIO_DETECT_LED, led_level);
	if (rs != ESP_OK) {
		ESP_LOGE(TAG, "Error from gpio_set_level: %s", esp_err_to_name(rs));			goto exit_on_fatal_error;
	}

	// Install GPIO ISR service. According to Espressif's
	// gpio_example_main.c example, 0 seems to be a good value for the
	// argument to gpio_install_isr_service.
	esp_err_t es_rs = gpio_install_isr_service(0);

	if (es_rs != ESP_OK) {
			ESP_LOGE(TAG, "Error from gpio_install_isr_service: %s",
					 esp_err_to_name(es_rs));
			goto exit_on_fatal_error;
		}

	// Let's configure the GPIO.
	gpio_config_t io_conf = {0};
	// Pin bit mask is 64-bit long => ULL suffix.
	io_conf.pin_bit_mask = (1ULL << GPIO_NUM_15);
	io_conf.mode = GPIO_MODE_INPUT;
	// Activate internal pull-up so that when the button is not pushed,
	// GPIO level is 3.3 V.
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	// Disable interrupts while configuring.
	io_conf.intr_type = GPIO_INTR_DISABLE;
	es_rs = gpio_config(&io_conf);
	if (es_rs != ESP_OK) {
		ESP_LOGE(TAG, "Error from gpio_config: %s",
				 esp_err_to_name(es_rs));
		goto exit_on_fatal_error;
	}
	// At this stage, GPIO configuration is OK.
	press_detected = false;
	// Now, configure interruption.
	// Assign the interrupt handler.
	es_rs = gpio_isr_handler_add(GPIO_NUM_15, gpio_interrupt_handler,
	                             NULL);
	if (es_rs != ESP_OK) {
	    ESP_LOGE(TAG, "Error from gpio_isr_handler_add: %s",
	            esp_err_to_name(es_rs));
	    goto exit_on_fatal_error;
	}
	// Now, enable interrupt for the GPIO.
	es_rs = gpio_set_intr_type(GPIO_NUM_15, GPIO_INTR_NEGEDGE);
	if (es_rs != ESP_OK) {
	    ESP_LOGE(TAG, "Error from gpio_set_intr_type: %s",
	            esp_err_to_name(es_rs));
	    goto exit_on_fatal_error;
	}

	// Now, check the global flag.
	while (true) {
	    if (!gpio_get_level(GPIO_SIDE_BUTTON)) {
	        //press_detected = false;
	        ESP_LOGI(TAG, "Press detected");
	        led_level = 1;
	        rs = gpio_set_level(GPIO_DETECT_LED, led_level);
	    }else{
	    	led_level=0;
	    	rs = gpio_set_level(GPIO_DETECT_LED, led_level);
	    }
	    if (rs != ESP_OK) {
	        		ESP_LOGE(TAG, "Error from gpio_set_level: %s", esp_err_to_name(rs));
	        		goto exit_on_fatal_error;
	        	}

        // Call a FreeRTOS function, to be sure not to trigger the task
        // watchdog.
        vTaskDelay(pdMS_TO_TICKS(CHECK_PERIOD_MS));
	}

	exit_on_fatal_error:
	ESP_LOGI(TAG, "Exiting");

}
