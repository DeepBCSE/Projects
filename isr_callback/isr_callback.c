/*
Copyright (c) 2025 Deep Bhuinya

Permission is hereby granted, free of charge, to any person obtaining a copy of this 
software and associated documentation files (the "Software"), to deal in the Software 
without restriction, including without limitation the rights to use, copy, modify, merge, 
publish, distribute, sublicense, and/or sell copies of the Software, and to permit 
persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies 
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.

Author:         Deep Bhuinya
Email:          deepb.cse@gmail.com
*/

/*
Question:
Design and implement an API that allows multiple clients to register for notifications
when new data from any generic sensor (temperature, GPS, accelerometer, etc.) is available.

Features:
- Multiple sensors
- Callback register mechanism
- Threaded Callbacks
- Memory-Constrained systems
- Low-Power optimization
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>

#define MAX_ALLOWED_CALLBACKS 10

typedef void (*PointerToFunc)(void *);

// Structure to store registered callbacks
typedef struct {
    PointerToFunc callback[MAX_ALLOWED_CALLBACKS];
    char sensor_type_for_callback[MAX_ALLOWED_CALLBACKS];
    int num_of_callbacks;
    pthread_mutex_t lock;
} CallbackRegisterBook;

// Structure to store sensor data
typedef struct {
    float temp_sensor_data;
    int acc_sensor_data[3];
    char gps_sensor_data[50];
} SensorData;

// Bitfield structure for GPIO register
typedef struct {
    uint8_t temp    : 1;
    uint8_t acc     : 1;
    uint8_t gps     : 1;
    uint8_t rsvd    : 5;
} GpioPinReg;

// Function Prototypes
void callbackRegisterBookInit(CallbackRegisterBook *);
void sensorDataInit(SensorData *);
void gpioPinRegInit(GpioPinReg *);
int callbackRegister(CallbackRegisterBook *, PointerToFunc, char);
void client1CallbackMethod(void *);
void client2CallbackMethod(void *);
void client3CallbackMethod(void *);
void gpioPinSet(GpioPinReg *, char);
void sensorIrqHandler(GpioPinReg *, char, SensorData *);
void readTempSensorData(float *);
void readAccSensorData(int *);
void readGpsSensorData(char *);
void callbackToRegisteredClients(CallbackRegisterBook *, SensorData *);
void *callbackThread(void *arg);

int main() {
    printf("\n======== Sensor Notification System Started ========\n\n");
    CallbackRegisterBook callback_booklet;
    SensorData sensor_data;
    GpioPinReg gpio_reg;

    callbackRegisterBookInit(&callback_booklet);
    sensorDataInit(&sensor_data);
    gpioPinRegInit(&gpio_reg);

    // Registering Clients
    callbackRegister(&callback_booklet, client1CallbackMethod, 'T');
    callbackRegister(&callback_booklet, client2CallbackMethod, 'A');
    callbackRegister(&callback_booklet, client3CallbackMethod, 'G');

    // Trigger interrupt when sensor data is ready
    sleep(1);  // Simulating data ready time
    printf("\n[SENSOR T] Data ready\n");
    gpioPinSet(&gpio_reg, 'T');
    printf("[INTERRUPT CONTROLLER] Triggering interrupt for sensor: T\n");
    sensorIrqHandler(&gpio_reg, 'T', &sensor_data);

    sleep(1);
    printf("[SENSOR A] Data ready\n");
    gpioPinSet(&gpio_reg, 'A');
    printf("[INTERRUPT CONTROLLER] Triggering interrupt for sensor: A\n");
    sensorIrqHandler(&gpio_reg, 'A', &sensor_data);

    sleep(1);
    printf("[SENSOR G] Data ready\n");
    gpioPinSet(&gpio_reg, 'G');
    printf("[INTERRUPT CONTROLLER] Triggering interrupt for sensor: G\n");
    sensorIrqHandler(&gpio_reg, 'G', &sensor_data);

    // Notify registered clients
    printf("[TASK SCHEDULER] Will trigger callback to registered clients shortly...\n\n");
    sleep(2);  // Simulating a time based task scheduler.
    callbackToRegisteredClients(&callback_booklet, &sensor_data);

    // Destroy the mutexes
    pthread_mutex_destroy(&(callback_booklet.lock));

    printf("\n======== Sensor Notification System Completed ========\n\n");

    return 0;
}

// Initialize callback registry
void callbackRegisterBookInit(CallbackRegisterBook *callback_booklet) {
    memset(callback_booklet->callback, 0, sizeof(callback_booklet->callback));
    memset(callback_booklet->sensor_type_for_callback, 0, sizeof(callback_booklet->sensor_type_for_callback));
    callback_booklet->num_of_callbacks = 0;
    pthread_mutex_init(&(callback_booklet->lock), NULL);
}

// Initialize sensor data
void sensorDataInit(SensorData *sensor_data) {
    sensor_data->temp_sensor_data = 0.0;
    memset(sensor_data->acc_sensor_data, 0, sizeof(sensor_data->acc_sensor_data));
    memset(sensor_data->gps_sensor_data, 0, sizeof(sensor_data->gps_sensor_data));
}

// Initialize GPIO register
void gpioPinRegInit(GpioPinReg *gpio_reg) {
    memset(gpio_reg, 0, sizeof(*gpio_reg));
}

// Register a client for sensor callbacks
int callbackRegister(CallbackRegisterBook *callback_booklet, PointerToFunc clientCallbackMethod, char sensor) {
    pthread_mutex_lock(&(callback_booklet->lock));

    if (callback_booklet->num_of_callbacks >= MAX_ALLOWED_CALLBACKS) {
        pthread_mutex_unlock(&(callback_booklet->lock));
        return -1;
    }

    int index = callback_booklet->num_of_callbacks;

    callback_booklet->callback[index] = clientCallbackMethod;
    callback_booklet->sensor_type_for_callback[index] = sensor;
    callback_booklet->num_of_callbacks++;
    printf("[Client %d] Registered callback for sensor: %c\n", index + 1, sensor);

    pthread_mutex_unlock(&(callback_booklet->lock));
    return 0;
}

// Simulated client callback methods
void client1CallbackMethod(void *temp_sensor_data) {  
    printf("[Client 1] Received callback for sensor: T, data: %.2f\n", *((float *)temp_sensor_data));
}

void client2CallbackMethod(void *acc_sensor_data) {
    int *data = (int *)acc_sensor_data;
    printf("[Client 2] Received callback for sensor: A, Data: x = %d, y = %d, z = %d\n", data[0], data[1], data[2]);
}

void client3CallbackMethod(void *gps_sensor_data) {
    printf("[Client 3] Received callback for sensor: G, Data: %s\n", (char *)gps_sensor_data);
}

// Simulate setting a GPIO pin when sensor data is ready
void gpioPinSet(GpioPinReg *gpio_reg, char sensor) {
    switch(sensor) {
        case 'T': gpio_reg->temp = 1; break;
        case 'A': gpio_reg->acc = 1; break;
        case 'G': gpio_reg->gps = 1; break;
    }
}

// Interrupt Service Routine (ISR) to read sensor data
void sensorIrqHandler(GpioPinReg *gpio_reg, char sensor, SensorData *sensor_data) {
    switch(sensor) {
        case 'T': gpio_reg->temp = 0; readTempSensorData(&(sensor_data->temp_sensor_data)); break;
        case 'A': gpio_reg->acc = 0; readAccSensorData(sensor_data->acc_sensor_data); break;
        case 'G': gpio_reg->gps = 0; readGpsSensorData(sensor_data->gps_sensor_data); break;
    }
    
    printf("[MCU] ISR done for sensor: %c\n\n", sensor);
}

// Simulated sensor data reads
void readTempSensorData(float *temp_sensor_data) { *temp_sensor_data = 25.67; }
void readAccSensorData(int *acc_sensor_data) { acc_sensor_data[0] = 1; acc_sensor_data[1] = 2; acc_sensor_data[2] = 3; }
void readGpsSensorData(char *gps_sensor_data) { sprintf(gps_sensor_data, "Lat: 12.34, Long: 56.78"); }

// Notify registered clients using threads
void callbackToRegisteredClients(CallbackRegisterBook *callback_booklet, SensorData *sensor_data) {
    if (callback_booklet->num_of_callbacks == 0) {
        return; 
    }

    pthread_t threads[callback_booklet->num_of_callbacks];

    for (int i = 0; i < callback_booklet->num_of_callbacks; i++) {
        void *data = NULL;
        switch(callback_booklet->sensor_type_for_callback[i]) {
            case 'T': data = &(sensor_data->temp_sensor_data); break;
            case 'A': data = sensor_data->acc_sensor_data; break;
            case 'G': data = sensor_data->gps_sensor_data; break;
        }
        if (data) {
            void **args = malloc(2 * sizeof(void *));
            args[0] = callback_booklet->callback[i];
            args[1] = data;
            pthread_create(&threads[i], NULL, callbackThread, args);
        }
    }
    for (int i = 0; i < callback_booklet->num_of_callbacks; i++) {
        pthread_join(threads[i], NULL);
    }
}

// Thread function to handle callbacks
void *callbackThread(void *arg) {
    PointerToFunc callback = ((PointerToFunc *)arg)[0];
    void *data = ((void **)arg)[1];
    callback(data);
    free(arg);
    return NULL;
}
