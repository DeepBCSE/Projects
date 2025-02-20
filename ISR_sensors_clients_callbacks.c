/*
You are given:
a device with some sensor attached (temperature, GPS, accelerometer, etc)
an interrupt you can implement that will fire when data is ready
a function that will read the sensor data e.g. float read_temperature_data()
Design and implement an API that will allow clients to register for notifications when new sensor data comes in.
*/


/* Sollution approach:
 - Clients register themeselves for callback
 - Clients are running in threads => Thread safety is needed for shared data among clients
 - Each client registers seperately, when the data is available, callback to all the registered clients
 - Each client will have their own callback function => Simulate using some print operation on the Data
 - read_temperature_data() => Simulate using a random number generator
 - Data Structure to store the callbacks: Array of pointer to function, max size is defined as a 10
 - Interrupt is raised from the sensor to the registered clients when the sensor data is ready
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

#define MAX_ALLOWED_CALLBACKS 10
#define TEMP_SENSOR 0  // Bit 0 in the GPIO_PIN_REG
#define ACC_SENSOR 1
#define GPS_SENSOR 2

volatile uint8_t GPIO_PIN_REG = 0x00;

typedef void (*pointer_to_func)(void *);

typedef struct callback_register_book {
    pointer_to_func callback[MAX_ALLOWED_CALLBACKS];
    char sensor_type_for_callback[MAX_ALLOWED_CALLBACKS];
    int num_of_callbacks;
    pthread_mutex_t lock;
} callback_register_book;

typedef struct sensor_data {
    float temp_sensor_data;
    int acc_sensor_data[3];
    char gps_sensor_data[50];
} sensor_data;

void callback_register_book_init(callback_register_book *callback_booklet) {
    for (int i = 0; i < MAX_ALLOWED_CALLBACKS; i++) {
        callback_booklet -> callback[i] = NULL;
        callback_booklet -> sensor_type_for_callback[i] = 0;
    }
    callback_booklet -> num_of_callbacks = 0;
    pthread_mutex_init(&(callback_booklet->lock), NULL);
}

void sensor_data_init(sensor_data *sensor_data) {
    sensor_data -> temp_sensor_data = 0.0;
    for (int i = 0; i < 3; i++) {
        sensor_data -> acc_sensor_data[i] = 0;
    }
    for (int i = 0; i < 50; i++) {
        sensor_data -> gps_sensor_data[i] = 0;
    }
}

void Client1CallbackMethod(void *temp_sensor_data) {  
    printf("Client 1: Received Temperature Sensor Data: %.2f\n", *((float *)temp_sensor_data));
}

void Client2CallbackMethod(void *acc_sensor_data) {
    printf("Client 2: Received Accelerometer Sensor Data: x = %d, y = %d, z = %d\n", ((int *)acc_sensor_data)[0], ((int *)acc_sensor_data)[1], ((int *)acc_sensor_data)[2]);
}

void Client3CallbackMethod(void *gps_sensor_data) {
    printf("Client 3: Received GPS Sensor Data: ");
    puts((char *)gps_sensor_data);
}

int CallbackRegister(callback_register_book *callback_booklet, pointer_to_func ClientCallbackMethod, char sensor) {
    pthread_mutex_lock(&(callback_booklet->lock));
    if (callback_booklet -> num_of_callbacks > MAX_ALLOWED_CALLBACKS) {
        pthread_mutex_unlock(&(callback_booklet -> lock));
        return -1;
    }
    callback_booklet -> callback[callback_booklet -> num_of_callbacks] = ClientCallbackMethod;
    callback_booklet -> sensor_type_for_callback[callback_booklet -> num_of_callbacks] = sensor;
    (callback_booklet -> num_of_callbacks)++;
    pthread_mutex_unlock(&(callback_booklet->lock));
    return 0;
}

void ReadTempSensorData(float *temp_sensor_data) {
    *temp_sensor_data = 25.67;
}

void ReadAccSensorData(int *acc_sensor_data) {
    acc_sensor_data[0] = 1;
    acc_sensor_data[1] = 2;
    acc_sensor_data[2] = 3;
}

void ReadGpsSensorData(char *gps_sensor_data) {
    sprintf(gps_sensor_data, "Latitude: 12.34, Longitude: 56.78");
}

void CallbackToRegisteredClients(callback_register_book *callback_booklet, sensor_data *sensor_data) {
    for (int i = 0; i < callback_booklet -> num_of_callbacks; i++) {
        switch(callback_booklet -> sensor_type_for_callback[i]) {
            case 'T':
                callback_booklet -> callback[i](&(sensor_data -> temp_sensor_data));
                break;
            case 'A':
                callback_booklet -> callback[i](sensor_data -> acc_sensor_data);
                break;
            case 'G':
                callback_booklet -> callback[i](sensor_data -> gps_sensor_data);
                break;
        }
    }
}

void GPIO_pin_set(char sensor) {  // Multiple Sensor, thread safety?
    switch(sensor) {
        case 'T':
            GPIO_PIN_REG |= (1 < TEMP_SENSOR);
            break;
        case 'A':
            GPIO_PIN_REG |= (1 < ACC_SENSOR);
            break;
        case 'G':
            GPIO_PIN_REG |= (1 < GPS_SENSOR);
            break;
    }
}

void Sensor_IRQHandler(char sensor, sensor_data *sensor_data) {  // Multiple Sensor, thread safety?
    switch(sensor) {
        case 'T':
            GPIO_PIN_REG &= ~(1 < TEMP_SENSOR);
            ReadTempSensorData(&(sensor_data -> temp_sensor_data));
            break;
        case 'A':
            GPIO_PIN_REG &= ~(1 < ACC_SENSOR);
            ReadAccSensorData(sensor_data -> acc_sensor_data);
            break;
        case 'G':
            GPIO_PIN_REG &= ~(1 < GPS_SENSOR);
            ReadGpsSensorData(sensor_data -> gps_sensor_data);
            break;
    }
}

int main() {
    // Create and Initializing the Data Structures
    callback_register_book callback_booklet;
    callback_register_book_init(&callback_booklet);
    sensor_data sensor_data;
    sensor_data_init(&sensor_data);

    // Clients registering themselves
    CallbackRegister(&callback_booklet, Client1CallbackMethod, 'T');
    CallbackRegister(&callback_booklet, Client2CallbackMethod, 'A');
    CallbackRegister(&callback_booklet, Client3CallbackMethod, 'G');

    // Raising Interrupt when data is ready
    GPIO_pin_set('T');  // Sensor sets the GPIO pins when the data is ready
    Sensor_IRQHandler('T', &sensor_data);  // Interrupt Controller raising the interrupt after GPIO detects the pin change.

    sleep(1);  // Simulating the time in the data availability
    GPIO_pin_set('A');
    Sensor_IRQHandler('A', &sensor_data);

    sleep(1);
    GPIO_pin_set('G');
    Sensor_IRQHandler('G', &sensor_data);

    // Registered clients getting callback by a task scheduler
    CallbackToRegisteredClients(&callback_booklet, &sensor_data);

    // Destroy the mutexes
    pthread_mutex_destroy(&(callback_booklet.lock));

    return 0;
}
