/*

  TNKernel real-time kernel

  Copyright © 2004, 2010 Yuri Tiomkin
  Copyright © 2011 Koshkin Sergey
  All rights reserved.

  Permission to use, copy, modify, and distribute this software in source
  and binary forms and its documentation for any purpose and without fee
  is hereby granted, provided that the above copyright notice appear
  in all copies and that both that copyright notice and this permission
  notice appear in supporting documentation.

  THIS SOFTWARE IS PROVIDED BY THE YURI TIOMKIN AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL YURI TIOMKIN OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

*/

/* ver 2.7  */

#ifndef  _TN_PORT_H_
#define  _TN_PORT_H_

#if defined (__ICCARM__)    // IAR ARM

#define align_attr_start
  #define align_attr_end
  
#ifndef INLINE_FORCED
  #define INLINE_FORCED   _Pragma("inline=forced")
#endif
  
#elif defined (__GNUC__)    //-- GNU Compiler

  #define align_attr_start
  #define align_attr_end     __attribute__((aligned(0x8)))
  #define tn_stack_t     __attribute__((aligned(8))) unsigned int
  
#ifndef INLINE_FORCED
  #define INLINE_FORCED   static inline __attribute__ ((always_inline))
#endif

#elif defined ( __CC_ARM )  //-- RealView Compiler

  #define align_attr_start   __align(8)
  #define align_attr_end
  #define tn_stack_t     __attribute__((aligned(8))) unsigned int
  
#ifndef INLINE_FORCED
  #define INLINE_FORCED   __forceinline
#endif

#else
  #error "Unknown compiler"
  #define align_attr_start
  #define align_attr_end
	#define INLINE_FORCED
  #define tn_stack_t     unsigned int
#endif

#define TN_TIMER_STACK_SIZE           64
#define TN_IDLE_STACK_SIZE            48
#define TN_MIN_STACK_SIZE             40      //--  +20 for exit func when ver GCC > 4

#define TN_BITS_IN_INT                32
#define TN_ALIG                       sizeof(void*)
#define MAKE_ALIG(a)                  ((sizeof(a)+(TN_ALIG-1))&(~(TN_ALIG-1)))
#define TN_PORT_STACK_EXPAND_AT_EXIT  16

#define TN_NUM_PRIORITY        TN_BITS_IN_INT  //-- 0..31  Priority 0 always is used by timers task
#define TN_WAIT_INFINITE       0xFFFFFFFF
#define TN_POLLING             0x0
#define TN_FILL_STACK_VAL      0xFFFFFFFF

#ifdef __cplusplus
extern "C"  {
#endif

void  tn_switch_context_request(void);
void  tn_switch_context_exit(void);
int   tn_cpu_save_sr(void);
void  tn_cpu_restore_sr(int sr);
void  tn_start_exe(void);
int   tn_inside_irq(void);
int   ffs_asm(unsigned int val);
void  tn_arm_disable_interrupts(void);
void  tn_arm_enable_interrupts(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif

/* - ARM port ----------------------------------------------------------------*/
#if defined __TARGET_ARCH_4T

/* - Assembler functions prototypes ------------------------------------------*/
#ifdef __cplusplus
extern "C"  {
#endif

void  tn_switch_context(void);
void  tn_cpu_irq_handler(void);
void  tn_cpu_irq_isr(void);
void  tn_cpu_fiq_isr(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif

/* - Interrupt processing - processor specific -------------------------------*/

#define BEGIN_CRITICAL_SECTION  int tn_save_status_reg = tn_cpu_save_sr();
#define END_CRITICAL_SECTION    tn_cpu_restore_sr(tn_save_status_reg);  \
                                if (!tn_inside_irq())                   \
                                  tn_switch_context();

#define BEGIN_DISABLE_INTERRUPT int tn_save_status_reg = tn_cpu_save_sr();
#define END_DISABLE_INTERRUPT   tn_cpu_restore_sr(tn_save_status_reg);

/* - Cortex-M3 port ----------------------------------------------------------*/
#elif defined __TARGET_ARCH_6_M || __TARGET_ARCH_6S_M || __TARGET_ARCH_7_M || __TARGET_ARCH_7E_M

/* - Assembler functions prototypes ------------------------------------------*/
#define  tn_switch_context()

/* - Interrupt processing - processor specific -------------------------------*/

#define BEGIN_CRITICAL_SECTION  int tn_save_status_reg = tn_cpu_save_sr();
#define END_CRITICAL_SECTION    tn_cpu_restore_sr(tn_save_status_reg);

#define BEGIN_DISABLE_INTERRUPT int tn_save_status_reg = tn_cpu_save_sr();
#define END_DISABLE_INTERRUPT   tn_cpu_restore_sr(tn_save_status_reg);

#else
  #define BEGIN_CRITICAL_SECTION
  #define END_CRITICAL_SECTION

  #define BEGIN_DISABLE_INTERRUPT
  #define END_DISABLE_INTERRUPT

  #error "Wrong processor architecture"
#endif

#endif
