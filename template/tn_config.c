/******************** Copyright (c) 2012. All rights reserved ******************
  File Name  :  tn_config.c
  Author     :  Koshkin Sergey
  Version    :  1.00
  Date       :  24/12/2012
  Description:  

  This software is supplied "AS IS" without warranties of any kind.

*******************************************************************************/
#include <tn.h>
#include <tn_port.h>

const unsigned int timer_stack_size   = 128;
const unsigned int idle_stack_size    = 48;
const unsigned int tn_min_stack_size  = 40;

volatile unsigned long  tn_idle_count;

align_attr_start unsigned int timer_task_stack[timer_stack_size] align_attr_end;
align_attr_start unsigned int idle_task_stack[idle_stack_size] align_attr_end;

/*-----------------------------------------------------------------------------*
  �������� :  idle_task_func
  �������� :  ����������, ��� ������ ������ ��������� � ������� RUNNABLE
  ���������:  par - 
  ���������:  ���.
*-----------------------------------------------------------------------------*/
TASK_FUNC idle_task_func(void *par)
{
  for(;;) {
    tn_idle_count++;
  }
}

#if defined  TNKERNEL_PORT_ARM
/*-----------------------------------------------------------------------------*
  �������� :  tn_cpu_irq_handler
  �������� :  ���� ���������� ����� ����� ������ ���������� � ARM �����������.
              � ���� ����������� ���������� ���������� ������ ���������� �
              ��������� ��������������� ����������.
  ���������:  ���.
  ���������:  ���.
*-----------------------------------------------------------------------------*/
void tn_cpu_irq_handler(void)
{
  VIC_dispatcher();
}
#endif

/*-----------------------------------------------------------------------------*
  �������� :  cpu_int_enable
  �������� :  ������������� ��������� ������ � �������� ���������� � �������.
  ���������:  hz  - ������� ���������� �������.
  ���������:  ���
*-----------------------------------------------------------------------------*/
void tn_cpu_int_enable(unsigned int hz)
{
  /* �������� ���������� ���������� ������� ������ �����, ����� �������������
     ���� �������� RTOS */
  systick_init(hz);
  /* ��������� ���������� � ���� ���������������� */
  tn_arm_enable_interrupts();
}

/*------------------------------ ����� ����� ---------------------------------*/
