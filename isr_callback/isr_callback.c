/*
 * MIT License
 *
 * Copyright (c) 2025 Deep Bhuinya
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * Question:
 * Design and implement an API that allows multiple clients to register for
 * notifications when new data from any generic sensor (temperature, GPS,
 * accelerometer, etc.) is available.
 *
 * Features:
 * - Multiple sensors
 * - Callback register mechanism
 * - Threaded Callbacks
 * - Memory-Constrained systems
 * - Low-Power optimization
 */

/*
 * Future Scope:
 * Different files for each clients, sensors etc
 */

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ALLOWED_CALLBACKS 10

typedef void (*pointer_to_func)(void *);

/* Structure to store registered callbacks */
struct callback_register_book {
	pointer_to_func callback[MAX_ALLOWED_CALLBACKS];
	char sensor_type_for_callback[MAX_ALLOWED_CALLBACKS];
	int num_of_callbacks;
	pthread_mutex_t lock;
};

/* Structure to store sensor data */
struct sensor_data {
	float tmp;
	int acc[3];
	char gps[50];
};

/* Bitfield structure for GPIO register */
struct gpio_pin_reg {
	uint8_t temp : 1;
	uint8_t acc : 1;
	uint8_t gps : 1;
	uint8_t rsvd : 5;
};

/* Function Prototypes */
void init_struct_callback_register_book(
    struct callback_register_book *callback_booklet);
void init_struct_sensor_data(struct sensor_data *sensor_data);
void init_struct_gpio_pin_reg(struct gpio_pin_reg *gpio_reg);
int register_callback(struct callback_register_book *callback_booklet,
		      pointer_to_func clientCallbackMethod, char sensor_type);
void callback_client1(void *tmp_sensor_data);
void callback_client2(void *acc_sensor_data);
void callback_client3(void *gps_sensor_data);
void set_gpio_pin(struct gpio_pin_reg *gpio_reg, char sensor_type);
void handle_sensor_irq(struct gpio_pin_reg *gpio_reg, char sensor,
		       struct sensor_data *sensor_data);
void read_tmp_data(float *tmp_sensor_data);
void read_acc_data(int *acc_sensor_data);
void read_gps_data(char *gps_sensor_data);
void callback_clients(struct callback_register_book *callback_booklet,
		      struct sensor_data *sensor_data);
void *threaded_callback_to_clients(void *arg);

int main()
{
	printf("\n======== Sensor Notification System Started ========\n\n");
	struct callback_register_book callback_booklet;
	struct sensor_data sensor_data;
	struct gpio_pin_reg gpio_reg;

	init_struct_callback_register_book(&callback_booklet);
	init_struct_sensor_data(&sensor_data);
	init_struct_gpio_pin_reg(&gpio_reg);

	/* Registering Clients */
	register_callback(&callback_booklet, callback_client1, 'T');
	register_callback(&callback_booklet, callback_client2, 'A');
	register_callback(&callback_booklet, callback_client3, 'G');

	/* Trigger interrupt when sensor data is ready */
	sleep(1); /* Simulating data ready time */
	printf("\n[SENSOR T] Data ready\n");
	set_gpio_pin(&gpio_reg, 'T');
	printf("[INTERRUPT CONTROLLER] Triggering interrupt for sensor: T\n");
	handle_sensor_irq(&gpio_reg, 'T', &sensor_data);

	sleep(1);
	printf("[SENSOR A] Data ready\n");
	set_gpio_pin(&gpio_reg, 'A');
	printf("[INTERRUPT CONTROLLER] Triggering interrupt for sensor: A\n");
	handle_sensor_irq(&gpio_reg, 'A', &sensor_data);

	sleep(1);
	printf("[SENSOR G] Data ready\n");
	set_gpio_pin(&gpio_reg, 'G');
	printf("[INTERRUPT CONTROLLER] Triggering interrupt for sensor: G\n");
	handle_sensor_irq(&gpio_reg, 'G', &sensor_data);

	/* Notify registered clients */
	printf(
	    "[TASK SCHEDULER] Will trigger callback to registered clients shortly...\n\n");
	sleep(2); /* Simulating a time based task scheduler */
	callback_clients(&callback_booklet, &sensor_data);

	/* Destroy the mutexes */
	pthread_mutex_destroy(&(callback_booklet.lock));

	printf("\n======== Sensor Notification System Completed ========\n\n");

	return 0;
}

/* Initialize callback registry */
void init_struct_callback_register_book(
    struct callback_register_book *callback_booklet)
{
	memset(callback_booklet->callback, 0,
	       sizeof(callback_booklet->callback));
	memset(callback_booklet->sensor_type_for_callback, 0,
	       sizeof(callback_booklet->sensor_type_for_callback));
	callback_booklet->num_of_callbacks = 0;
	pthread_mutex_init(&(callback_booklet->lock), NULL);
}

/* Initialize sensor data */
void init_struct_sensor_data(struct sensor_data *sensor_data)
{
	sensor_data->tmp = 0.0;
	memset(sensor_data->acc, 0, sizeof(sensor_data->acc));
	memset(sensor_data->gps, 0, sizeof(sensor_data->gps));
}

/* Initialize GPIO register */
void init_struct_gpio_pin_reg(struct gpio_pin_reg *gpio_reg)
{
	memset(gpio_reg, 0, sizeof(*gpio_reg));
}

/* Register a client for sensor callbacks */
int register_callback(struct callback_register_book *callback_booklet,
		      pointer_to_func clientCallbackMethod, char sensor_type)
{
	pthread_mutex_lock(&(callback_booklet->lock));

	if (callback_booklet->num_of_callbacks >= MAX_ALLOWED_CALLBACKS) {
		pthread_mutex_unlock(&(callback_booklet->lock));
		return -1;
	}

	int index = callback_booklet->num_of_callbacks;

	callback_booklet->callback[index] = clientCallbackMethod;
	callback_booklet->sensor_type_for_callback[index] = sensor_type;
	callback_booklet->num_of_callbacks++;
	printf("[Client %d] Registered callback for sensor: %c\n", index + 1,
	       sensor_type);

	pthread_mutex_unlock(&(callback_booklet->lock));
	return 0;
}

/* Simulated client callback methods */
void callback_client1(void *tmp_sensor_data)
{
	printf("[Client 1] Received callback for sensor: T, Data: %.2f\n",
	       *((float *)tmp_sensor_data));
}

void callback_client2(void *acc_sensor_data)
{
	int *data = (int *)acc_sensor_data;
	printf(
	    "[Client 2] Received callback for sensor: A, Data: x = %d, y = %d, z = %d\n",
	    data[0], data[1], data[2]);
}

void callback_client3(void *gps_sensor_data)
{
	printf("[Client 3] Received callback for sensor: G, Data: %s\n",
	       (char *)gps_sensor_data);
}

/* Simulate setting a GPIO pin when sensor data is ready */
void set_gpio_pin(struct gpio_pin_reg *gpio_reg, char sensor_type)
{
	switch (sensor_type) {
	case 'T':
		gpio_reg->temp = 1;
		break;
	case 'A':
		gpio_reg->acc = 1;
		break;
	case 'G':
		gpio_reg->gps = 1;
		break;
	}
}

/* Interrupt Service Routine (ISR) to read sensor data */
void handle_sensor_irq(struct gpio_pin_reg *gpio_reg, char sensor,
		       struct sensor_data *sensor_data)
{
	switch (sensor) {
	case 'T':
		gpio_reg->temp = 0;
		read_tmp_data(&(sensor_data->tmp));
		break;
	case 'A':
		gpio_reg->acc = 0;
		read_acc_data(sensor_data->acc);
		break;
	case 'G':
		gpio_reg->gps = 0;
		read_gps_data(sensor_data->gps);
		break;
	}

	printf("[MCU] ISR done for sensor: %c\n\n", sensor);
}

/* Simulated sensor data reads */
void read_tmp_data(float *tmp_sensor_data)
{
	*tmp_sensor_data = 25.67;
}

void read_acc_data(int *acc_sensor_data)
{
	acc_sensor_data[0] = 1;
	acc_sensor_data[1] = 2;
	acc_sensor_data[2] = 3;
}

void read_gps_data(char *gps_sensor_data)
{
	sprintf(gps_sensor_data, "Lat: 12.34, Long: 56.78");
}

/* Notify registered clients using threads */
void callback_clients(struct callback_register_book *callback_booklet,
		      struct sensor_data *sensor_data)
{
	if (callback_booklet->num_of_callbacks == 0) {
		return;
	}

	pthread_t threads[callback_booklet->num_of_callbacks];

	for (int i = 0; i < callback_booklet->num_of_callbacks; i++) {
		void *data = NULL;
		switch (callback_booklet->sensor_type_for_callback[i]) {
		case 'T':
			data = &(sensor_data->tmp);
			break;
		case 'A':
			data = sensor_data->acc;
			break;
		case 'G':
			data = sensor_data->gps;
			break;
		}
		if (data) {
			void **args = malloc(2 * sizeof(void *));
			args[0] = callback_booklet->callback[i];
			args[1] = data;
			pthread_create(&threads[i], NULL,
				       threaded_callback_to_clients, args);
		}
	}
	for (int i = 0; i < callback_booklet->num_of_callbacks; i++) {
		pthread_join(threads[i], NULL);
	}
}

/* Thread function to handle callbacks */
void *threaded_callback_to_clients(void *arg)
{
	pointer_to_func callback = ((pointer_to_func *)arg)[0];
	void *data = ((void **)arg)[1];
	callback(data);
	free(arg);
	return NULL;
}
