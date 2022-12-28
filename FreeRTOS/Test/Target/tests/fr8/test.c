/*
 * FreeRTOS Kernel <DEVELOPMENT BRANCH>
 * Copyright (C) 2022 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/**
 * @file test.c
 * @brief Implements FR8 test functions for SMP on target testing.
 */

/* Kernel includes. */
#include "FreeRTOS.h" /* Must come first. */
#include "queue.h"    /* RTOS queue related API prototypes. */
#include "semphr.h"   /* Semaphore related API prototypes. */
#include "task.h"     /* RTOS task related API prototypes. */
#include "timers.h"   /* Software timer related API prototypes. */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "bsl.h"
#include "unity.h" /* unit testing support functions */

/* Priorities at which the tasks are created.  The max priority can be specified
as ( configMAX_PRIORITIES - 1 ). */
#define mainTASK_A_PRIORITY (tskIDLE_PRIORITY + 1)
#define mainTASK_B_PRIORITY (tskIDLE_PRIORITY + 2)

static void prvTaskA(void *pvParameters);
static void prvTaskB(void *pvParameters);

#if configNUMBER_OF_CORES != 2
#error Require two cores be configured for FreeRTOS
#endif

void setup_test_fr8_001(void) {
  xTaskCreate(prvTaskA, "TaskA", configMINIMAL_STACK_SIZE, NULL,
              mainTASK_A_PRIORITY, NULL);

  xTaskCreate(prvTaskB, "TaskB", configMINIMAL_STACK_SIZE, NULL,
              mainTASK_B_PRIORITY, NULL);
}

/* Is run before every test, put unit init calls here. */
void setUp(void)
{
}

/* Is run after every test, put unit clean-up calls here. */
void tearDown(void)
{
}

int main(void) {
  vPortInitTestEnvironment();

  setup_test_fr8_001();

  vTaskStartScheduler();

  /* should never reach here */
  panic_unsupported();

  return 0;
}

static BaseType_t xTaskBState = 0;
static BaseType_t xIsrAssertionComplete = pdFALSE;
static BaseType_t xIsrObservedTaskBInsideCriticalSection = pdFALSE;
static BaseType_t xInsideTaskBCriticalSection = pdFALSE;

static void softwareInterruptHandlerSimple(void) {
  BaseType_t xIter;
  UBaseType_t uxSavedInterruptStatus;

  for(xIter = 1; xIter < 10; xIter++)
  {
    uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    if (xInsideTaskBCriticalSection)
    {
      xIsrObservedTaskBInsideCriticalSection = true;
    }
    vPortBusyWaitMicroseconds((uint32_t)10000);
    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
  }

  xIsrAssertionComplete = true;
}

static void prvTaskA(void *pvParameters) {
  BaseType_t xHandlerNum = -1;

  // wait for Task B to get to 6 itertions
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(10));
    if (xTaskBState > 5) {
      break;
    }
  }

  xHandlerNum = xPortRegisterSoftwareInterruptHandler(softwareInterruptHandlerSimple);
  vPortTriggerSoftwareInterrupt(xHandlerNum);

  // idle the task
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

static void fr08_validateOnlyOneCriticalSectionRanAtATime(void)
{
  TEST_ASSERT_TRUE(xIsrAssertionComplete && !xIsrObservedTaskBInsideCriticalSection);
}

static void prvTaskB(void *pvParameters) {
  BaseType_t xIter = 1;

  for (xIter = 1; xIter < 10; xIter++) {
    vTaskDelay(pdMS_TO_TICKS(10));
    while (xTaskBState == 6 && !xIsrAssertionComplete) {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    taskENTER_CRITICAL();
    xInsideTaskBCriticalSection = true;
    vPortBusyWaitMicroseconds((uint32_t)10000);
    xTaskBState++;
    xInsideTaskBCriticalSection = false;
    taskEXIT_CRITICAL();
  }

  while (!xIsrAssertionComplete) {
      vTaskDelay(pdMS_TO_TICKS(10));
  }

  UNITY_BEGIN();

  RUN_TEST(fr08_validateOnlyOneCriticalSectionRanAtATime);

  UNITY_END();

  // idle the task
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
