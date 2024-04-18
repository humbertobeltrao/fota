#include <stdio.h>
#include <signal.h>
#include <alchemy/task.h>
#include <alchemy/sem.h>
#include <alchemy/timer.h>
#include <stdlib.h>

#define NTASKS 2
#define HIGH 52 /* high priority */
#define LOW 50  /* low priority */

RT_TASK demo_task[NTASKS];
RT_SEM mysync;

#define BLINK_PERIOD 500000000 // Blink period in nanoseconds (0.5 seconds)

void controlLED(int gpio_pin) {
   int led_state = 0;

    // Export the GPIO pin
    char gpio_export_command[64];
    snprintf(gpio_export_command, sizeof(gpio_export_command), "echo %d > /sys/class/gpio/export", gpio_pin);
    system(gpio_export_command);

    // Set the GPIO direction to out
    char gpio_direction_command[64];
    snprintf(gpio_direction_command, sizeof(gpio_direction_command), "echo out > /sys/class/gpio/gpio%d/direction", gpio_pin);
    system(gpio_direction_command);

    while (1) {
        // Toggle the LED state
        led_state = !led_state;
        // Set the GPIO pin value
        char gpio_value_command[64];
        snprintf(gpio_value_command, sizeof(gpio_value_command), "echo %d > /sys/class/gpio/gpio%d/value", led_state, gpio_pin);
        system(gpio_value_command);


        // Sleep for the remaining half period
        rt_task_sleep(BLINK_PERIOD / 2);
    }

     // Unexport the GPIO pin (cleanup)
    char gpio_unexport_command[64];
    snprintf(gpio_unexport_command, sizeof(gpio_unexport_command), "echo %d > /sys/class/gpio/unexport", gpio_pin);
    system(gpio_unexport_command);
}

void demo(void *arg) {
    int num = *(int *)arg;
    printf("Task: %d\n", num);
    rt_sem_p(&mysync, TM_INFINITE);
    printf("Blinking LED %d\n", num);

    if (num == 0) {
        // Control P8_12 (GPIO 44)
        controlLED(44);
    } else {
        // Control P8_16 (GPIO 46)
        controlLED(46);
    }

    printf("End Task: %d\n", num);
}

void startup() {
    int i;
    char str[20];

    // Semaphore to sync task startup on
    rt_sem_create(&mysync, "MySemaphore", 0, S_FIFO);

    for (i = 0; i < NTASKS; i++) {
        printf("Start Task: %d\n", i);
        sprintf(str, "task%d", i);
        rt_task_create(&demo_task[i], str, 0, (i == 0) ? LOW : HIGH, 0);
        rt_task_start(&demo_task[i], &demo, &i);
    }

    printf("Wake up all tasks\n");
    rt_sem_broadcast(&mysync);
}

int main(int argc, char* argv[]) {
    startup();
    printf("\nType CTRL-C to end this program\n\n");
    pause();
}