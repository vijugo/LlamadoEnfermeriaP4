#ifndef FAST_TASK_H
#define FAST_TASK_H

/**
 * @brief Initialize a high-priority task triggered every 100 microseconds
 * using a hardware GPTimer.
 */
void fast_task_init(void);

#endif // FAST_TASK_H
