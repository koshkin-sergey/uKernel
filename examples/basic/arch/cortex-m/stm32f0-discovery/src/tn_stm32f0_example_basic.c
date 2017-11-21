/*
 * Copyright (C) 2015 Sergey Koshkin <koshkin.sergey@gmail.com>
 * All rights reserved
 *
 * File Name  :	tn_stm32f0_example_basic.c
 * Description:	tn_stm32f0_example_basic
 */

/*******************************************************************************
 *  includes
 ******************************************************************************/

#include <stddef.h>
#include <stdint.h>
#include "ukernel.h"
#include "stm32f0xx.h"
#include "system_stm32f0xx.h"

/*******************************************************************************
 *  external declarations
 ******************************************************************************/

/*******************************************************************************
 *  defines and macros (scope: module-local)
 ******************************************************************************/

#define HZ    1000

#define TASK_A_STK_SIZE   64
#define TASK_B_STK_SIZE   64

#define TASK_A_PRIORITY   1
#define TASK_B_PRIORITY   2

/*******************************************************************************
 *  typedefs and structures (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  global variable definitions  (scope: module-exported)
 ******************************************************************************/

/*******************************************************************************
 *  global variable definitions (scope: module-local)
 ******************************************************************************/

static TN_TCB task_A;
static TN_TCB task_B;

tn_stack_t task_A_stack[TASK_A_STK_SIZE];
tn_stack_t task_B_stack[TASK_B_STK_SIZE];

static volatile bool done_a;

/*******************************************************************************
 *  function prototypes (scope: module-local)
 ******************************************************************************/

/*******************************************************************************
 *  function implementations (scope: module-local)
 ******************************************************************************/

/*-----------------------------------------------------------------------------*
 * Название :
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
static void task_A_func(void *param)
{
  done_a = false;

  while (done_a != true) {
    GPIOC->ODR ^= (1UL << 8);
    osTaskSleep(100);
  }

  GPIOC->ODR &= ~(1UL << 8);
}

/*-----------------------------------------------------------------------------*
 * Название :
 * Описание :
 * Параметры:
 * Результат:
 *----------------------------------------------------------------------------*/
static void task_B_func(void *param)
{
  for (;;) {
    GPIOC->ODR ^= (1UL << 9);
    if ((GPIOC->ODR & (1UL << 9)) != 0)
      done_a = true;
    else
      osTaskActivate(&task_A);

    osTaskSleep(2000);
  }
}

/*-----------------------------------------------------------------------------*
  Название :  app_init
  Описание :  Инициализация задач, очередей и др. объектов ОС
  Параметры:  Нет
  Результат:  Нет
*-----------------------------------------------------------------------------*/
static void app_init(void)
{
  /* Инициализируем железо */
  RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

  GPIOC->MODER |= (GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0);

  osTaskCreate(
    &task_A,                   // TCB задачи
    task_A_func,               // Функция задачи
    TASK_A_PRIORITY,           // Приоритет задачи
    &(task_A_stack[TASK_A_STK_SIZE-1]),
    TASK_A_STK_SIZE,           // Размер стека (в int, не в байтах)
    NULL,                      // Параметры функции задачи
    TN_TASK_START_ON_CREATION  // Параметр создания задачи
  );

  osTaskCreate(
    &task_B,                   // TCB задачи
    task_B_func,               // Функция задачи
    TASK_B_PRIORITY,           // Приоритет задачи
    &(task_B_stack[TASK_B_STK_SIZE-1]),
    TASK_B_STK_SIZE,           // Размер стека (в int, не в байтах)
    NULL,                      // Параметры функции задачи
    TN_TASK_START_ON_CREATION  // Параметр создания задачи
  );
}

/*******************************************************************************
 *  function implementations (scope: module-exported)
 ******************************************************************************/

/*-----------------------------------------------------------------------------*
  Название :  main
  Описание :  Start RTOS
  Параметры:  Нет
  Результат:  Нет
*-----------------------------------------------------------------------------*/
int main(void)
{
  TN_OPTIONS  tn_opt;

  /* Старт операционной системы */
  tn_opt.app_init       = app_init;
  tn_opt.freq_timer     = HZ;
  tn_start_system(&tn_opt);

  return (-1);
}

/*-----------------------------------------------------------------------------*
 * Название : osSysTickInit
 * Описание : Конфигурирует  и включает системный таймер в системе.
 * Параметры: hz - Частота системного таймера.
 * Результат: Нет
 *----------------------------------------------------------------------------*/
void osSysTickInit(unsigned int hz)
{
  /* Включаем прерывание системного таймера именно здесь, после инициализации
     всех сервисов RTOS */
  SystemCoreClockUpdate();
  SysTick_Config(SystemCoreClock/hz);
}

/*-----------------------------------------------------------------------------*
  Название :  SysTick_Handler
  Описание :  Обработчик прерывания системного таймера вызывается каждую 1 мс.
              Используется для генерации тиков операционной и графической систем.
  Параметры:  Нет
  Результат:  Нет
*-----------------------------------------------------------------------------*/
void SysTick_Handler(void)
{
  osTimerHandle();
}

/* ----------------------------- End of file ---------------------------------*/
