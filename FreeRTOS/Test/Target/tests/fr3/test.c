/* Kernel includes. */

#include "FreeRTOS.h" /* Must come first. */
#include "queue.h"    /* RTOS queue related API prototypes. */
#include "semphr.h"   /* Semaphore related API prototypes. */
#include "task.h"     /* RTOS task related API prototypes. */
#include "timers.h"   /* Software timer related API prototypes. */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bsl.h"
#include "unity.h" /* unit testing support functions */

/* Priorities at which the tasks are created.  The max priority can be specified
as ( configMAX_PRIORITIES - 1 ). */
#define mainTASK_A_PRIORITY (tskIDLE_PRIORITY + 1)
#define mainTASK_B_PRIORITY (tskIDLE_PRIORITY + 1)
#define mainTASK_C_PRIORITY (tskIDLE_PRIORITY + 1)

static void prvTaskA(void *pvParameters);
static void prvTaskB(void *pvParameters);
static void prvTaskC(void *pvParameters);

#if configNUMBER_OF_CORES != 2
#error Require two cores be configured for FreeRTOS
#endif

bool testFailed = false;
bool testPassed = false;

void test_fr3TASK_SWITCHED_IN(void) {
  UBaseType_t idx, numTasksRunning;
  TaskStatus_t taskStatus[16];
  UBaseType_t taskStatusArraySize = 16;
  unsigned long totalRunTime;
  int coreIndex = 0;
  SchedTraceLogRow *logRow;
  int retcode = 0;

  static int taskSwitchCount = 0;
  static bool taskARan = false;
  static bool taskBRan = false;
  static bool taskCRan = false;

  if (!(testPassed || testFailed))
  {
    numTasksRunning = uxTaskGetSystemState((TaskStatus_t * const)&taskStatus, taskStatusArraySize, &totalRunTime);

    for(idx = 0; idx < numTasksRunning; idx++)
    {
      if ((strcmp(taskStatus[idx].pcTaskName, "TaskA") == 0) && (taskStatus[idx].eCurrentState == eRunning))
      {
        taskARan = true;
      }
      if ((strcmp(taskStatus[idx].pcTaskName, "TaskB") == 0) && (taskStatus[idx].eCurrentState == eRunning))
      {
        taskBRan = true;
      }
      if ((strcmp(taskStatus[idx].pcTaskName, "TaskC") == 0) && (taskStatus[idx].eCurrentState == eRunning))
      {
        taskCRan = true;
      }
    }

    taskSwitchCount++;
    if (taskSwitchCount > 2048)
    {
      if (taskARan && taskBRan && taskCRan) {
        testPassed = true;
      } else {
        testFailed = true;
      }
    }
  }
}

void setup_test_fr3_001(void) {
  xTaskCreate(prvTaskA, "TaskA", configMINIMAL_STACK_SIZE, NULL,
              mainTASK_A_PRIORITY, NULL);

  xTaskCreate(prvTaskB, "TaskB", configMINIMAL_STACK_SIZE, NULL,
              mainTASK_B_PRIORITY, NULL);

  xTaskCreate(prvTaskC, "TaskC", configMINIMAL_STACK_SIZE, NULL,
              mainTASK_C_PRIORITY, NULL);
}

void setUp(void) {} /* Is run before every test, put unit init calls here. */
void tearDown(void) {
} /* Is run after every test, put unit clean-up calls here. */

int main(void) {
  initTestEnvironment();

  setup_test_fr3_001();

  vTaskStartScheduler();

  /* should never reach here */
  panic_unsupported();

  return 0; 
}

static void prvTaskA(void *pvParameters) {
  // idle the task
  for (;;) {
    busyWaitMicroseconds(100000);
  }
}

static void prvTaskB(void *pvParameters) {
  // idle the task
  for (;;) {
    busyWaitMicroseconds(100000);
  }
}

static void fr03_validateAllTasksHaveRun(void) {
  int attempt;

  for(attempt=1; attempt<100; attempt++)
  {
    if (testPassed || testFailed)
    {
      break;
    }

    busyWaitMicroseconds(10000);
  }

  TEST_ASSERT_TRUE((testPassed && !testFailed));

  if (testPassed && !testFailed)
  {
      setPin(LED_PIN);
      sendReport(testPassedString, testPassedStringLen);
  } else
  {
      sendReport(testFailedString, testFailedStringLen);
  }
}

static void prvTaskC(void *pvParameters) {

  UNITY_BEGIN();

  RUN_TEST(fr03_validateAllTasksHaveRun);

  UNITY_END();

  // idle the task
  for (;;) {
    busyWaitMicroseconds(100000);
  }
}
