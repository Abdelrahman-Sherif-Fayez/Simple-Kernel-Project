#include <stdint.h>
#include "kernel.h"
#include "board_support_package.h"

uint32_t stack_TaskA[40];
OSTask TaskA;
void Green_Led_TaskA() {
    while (1) {
        BSP_ledGreenOn();
        OS_delay(BSP_TICKS_PER_SEC / 4U);
        BSP_ledGreenOff();
        OS_delay(BSP_TICKS_PER_SEC * 3U / 4U);
    }
}

uint32_t stack_TaskB[40];
OSTask TaskB;
void Blue_Led_TaskB() {
    while (1) {
        BSP_ledBlueOn();
        OS_delay(BSP_TICKS_PER_SEC / 2U);
        BSP_ledBlueOff();
        OS_delay(BSP_TICKS_PER_SEC / 3U);
    }
}

uint32_t stack_idletask[40];

int main() {
    BSP_init();
    OS_init(stack_idletask, sizeof(stack_idletask));

    /* start TaskA */
    OSTask_start(&TaskA,
                   &Green_Led_TaskA,
                   stack_TaskA, sizeof(stack_TaskA));

    /* start TaskB */
    OSTask_start(&TaskB,
                   &Blue_Led_TaskB,
                   stack_TaskB, sizeof(stack_TaskB));


    /* transfer control to the RTOS to run the tasks */
    OS_run();

		//Code shouldn't reach here
		while(1){};
}
