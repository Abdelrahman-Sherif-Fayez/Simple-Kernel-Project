# Simple-Kernel-Project
Simple kernel that uses Round Robin Scheduling Algorithm to multi-task two periodic tasks which are Green_Led_TaskA and Blue_Led_TaskB as is they were running simultaneously by giving each task a systick time slice after which the Os scheduler is called to schedule the next Task.

All Green_Led_TaskA does is that it turns on the green led on tiva-c board for some delay then it turns it off for some delay and loop again, similarly Blue_Led_TaskB but with the blue led.

Also, we called the tasks periodic as I implemented an Os_delay api for the delay of the task, in which it changes the state of the task as blocked (not ready) and calls the scheduler to schedule the next task automatically, and when all tasks are blocked it schedules an idle task which turns on and off the red led then saves the CPU clock cycles and power by turning it to the power saving mode until an interrupt comes using WFI instruction.
