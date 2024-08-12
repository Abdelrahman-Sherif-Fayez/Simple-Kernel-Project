#include <stdint.h>

#ifndef KERNEL_H
#define KERNEL_H

/* Task Control Block (TCB) */
typedef struct {
    void *sp; /* stack pointer */
    uint32_t timeout; /* timeout delay down-counter */
    /* ... other attributes associated with a task */
} OSTask;

typedef void (*OSTaskHandler)();

void OS_init(void *stkSto, uint32_t stkSize);

/* callback to handle the idle condition */
void OS_onIdle(void);

/* this function must be called with interrupts DISABLED */
void OS_sched(void);

/* transfer control to the RTOS to run the tasks */
void OS_run(void);

/* blocking delay */
void OS_delay(uint32_t ticks);

/* process all timeouts */
void OS_tick(void);

/* callback to configure and start interrupts */
void OS_onStartup(void);

void OSTask_start(
    OSTask *me,
    OSTaskHandler taskHandler,
    void *stkSto, uint32_t stkSize);

#endif /* KERNEL_H */

