#include <stdint.h>
#include "kernel.h"
#include "qassert.h"

#define PENDSVREG *(uint32_t volatile *)0xE000ED04

Q_DEFINE_THIS_FILE

OSTask * volatile OS_curr; /* pointer to the current task */
OSTask * volatile OS_next; /* pointer to the next task to run */

OSTask *OS_tasks[32 + 1]; /* array of tasks started so far */
uint8_t OS_tasksNum; /* number of tasks started so far */
uint8_t OS_currIdx; /* current task index for round robin scheduling */
uint32_t OS_readySet; /* bitmask of tasks that are ready to run */

OSTask idletask;
void main_idletask() {
    while (1) {
        OS_onIdle();
    }
}

void OS_init(void *stkSto, uint32_t stkSize) {
    /* set the PendSV interrupt priority to the lowest level 0xFF */
    *(uint32_t volatile *)0xE000ED20 |= (0xFFU << 16);

    /* start idletask task */
    OSTask_start(&idletask,
                   &main_idletask,
                   stkSto, stkSize);
}

void OS_sched(void) {
    /* OS_next = ... */
    if (OS_readySet == 0U) { /* idle condition? */
        OS_currIdx = 0U; /* index of the idle task */
    }
    else {
        do {
            ++OS_currIdx;
            if (OS_currIdx == OS_tasksNum) {
                OS_currIdx = 1U;
            }
        } while ((OS_readySet & (1U << (OS_currIdx - 1U))) == 0U);
    }
    /* temporarty for the next task */
    OSTask * const next = OS_tasks[OS_currIdx];

    /* trigger PendSV, if needed */
    if (next != OS_curr) {
        OS_next = next;
        PENDSVREG = (1U << 28);
    }
}

void OS_run(void) {
    /* callback to configure and start interrupts */
    OS_onStartup();

    __asm volatile ("cpsid i");
    OS_sched();
    __asm volatile ("cpsie i");

    /* the following code should never execute */
    Q_ERROR();
}

void OS_tick(void) {
    uint8_t n;
    for (n = 1U; n < OS_tasksNum; ++n) {
        if (OS_tasks[n]->timeout != 0U) {
            --OS_tasks[n]->timeout;
            if (OS_tasks[n]->timeout == 0U) {
                OS_readySet |= (1U << (n - 1U));
            }
        }
    }
}

void OS_delay(uint32_t ticks) {
    __asm volatile ("cpsid i");

    /* never call OS_delay from the idletask */
    Q_REQUIRE(OS_curr != OS_tasks[0]);

    OS_curr->timeout = ticks;
    OS_readySet &= ~(1U << (OS_currIdx - 1U));
    OS_sched();
    __asm volatile ("cpsie i");
}

void OSTask_start(
    OSTask *me,
    OSTaskHandler taskHandler,
    void *stkSto, uint32_t stkSize)
{
    /* round down the stack top to the 8-byte boundary
    * NOTE: ARM Cortex-M stack grows down from hi -> low memory
    */
    uint32_t *sp = (uint32_t *)((((uint32_t)stkSto + stkSize) / 8) * 8);
    uint32_t *stk_limit;

    *(--sp) = (1U << 24);  /* xPSR */
    *(--sp) = (uint32_t)taskHandler; /* PC */
    *(--sp) = 0x0000000EU; /* LR  */
    *(--sp) = 0x0000000CU; /* R12 */
    *(--sp) = 0x00000003U; /* R3  */
    *(--sp) = 0x00000002U; /* R2  */
    *(--sp) = 0x00000001U; /* R1  */
    *(--sp) = 0x00000000U; /* R0  */
    /* additionally, fake registers R4-R11 */
    *(--sp) = 0x0000000BU; /* R11 */
    *(--sp) = 0x0000000AU; /* R10 */
    *(--sp) = 0x00000009U; /* R9 */
    *(--sp) = 0x00000008U; /* R8 */
    *(--sp) = 0x00000007U; /* R7 */
    *(--sp) = 0x00000006U; /* R6 */
    *(--sp) = 0x00000005U; /* R5 */
    *(--sp) = 0x00000004U; /* R4 */

    /* save the top of the stack in the task's attibute */
    me->sp = sp;

    /* round up the bottom of the stack to the 8-byte boundary */
    stk_limit = (uint32_t *)(((((uint32_t)stkSto - 1U) / 8) + 1U) * 8);

    /* pre-fill the unused part of the stack with 0xBBBB0000 */
    for (sp = sp - 1U; sp >= stk_limit; --sp) {
        *sp = 0xBBBB0000U;
    }

    Q_ASSERT(OS_tasksNum < Q_DIM(OS_tasks));

    /* register the task with the OS */
    OS_tasks[OS_tasksNum] = me;
    /* make the task ready to run */
    if (OS_tasksNum > 0U) {
        OS_readySet |= (1U << (OS_tasksNum - 1U));
    }
    ++OS_tasksNum;
}

__attribute__ ((naked))
void PendSV_Handler(void) {
__asm volatile (
    /* __disable_irq(); */
    "  CPSID         I                 \n"

    /* if (OS_curr != (OSTask *)0) { */
    "  LDR           r1,=OS_curr       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  CBZ           r1,PendSV_restore \n"

    /*     push registers r4-r11 on the stack */
    "  PUSH          {r4-r11}          \n"

    /*     OS_curr->sp = sp; */
    "  LDR           r1,=OS_curr       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  STR           sp,[r1,#0x00]     \n"
    /* } */

    "PendSV_restore:                   \n"
    /* sp = OS_next->sp; */
    "  LDR           r1,=OS_next       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  LDR           sp,[r1,#0x00]     \n"

    /* OS_curr = OS_next; */
    "  LDR           r1,=OS_next       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  LDR           r2,=OS_curr       \n"
    "  STR           r1,[r2,#0x00]     \n"

    /* pop registers r4-r11 */
    "  POP           {r4-r11}          \n"

    /* __enable_irq(); */
    "  CPSIE         I                 \n"

    /* return to the next task */
    "  BX            lr                \n"
    );
}
