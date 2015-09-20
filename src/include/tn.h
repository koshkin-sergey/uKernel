/*

 TNKernel real-time kernel

 Copyright � 2004, 2010 Yuri Tiomkin
 Copyright � 2013, 2015 Sergey Koshkin <koshkin.sergey@gmail.com>
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

#ifndef _TH_H_
#define _TH_H_

#include "tn_port.h"
#include "tn_winfo.h"

#define TASK_FUNC __declspec(noreturn) void
#define TNKERNEL_VERSION	3000000

/* - The system configuration (change it for your particular project) --------*/
#define TN_CHECK_PARAM        1
#define TN_MEAS_PERFORMANCE   1
#define USE_ASM_FFS           1
#define USE_MUTEXES           1
#define USE_EVENTS            1
#define USE_INLINE_CDLL       1

/* - Constants ---------------------------------------------------------------*/
#define TN_ST_STATE_NOT_RUN              0 
#define TN_ST_STATE_RUNNING              1 

#define TN_TASK_START_ON_CREATION        1 
#define TN_TASK_TIMER                 0x80 
#define TN_TASK_IDLE                  0x40 

#define TN_ID_TASK              ((int)0x47ABCF69) 
#define TN_ID_SEMAPHORE         ((int)0x6FA173EB) 
#define TN_ID_EVENT             ((int)0x5E224F25) 
#define TN_ID_DATAQUEUE         ((int)0x8C8A6C89) 
#define TN_ID_FSMEMORYPOOL      ((int)0x26B7CE8B) 
#define TN_ID_MUTEX             ((int)0x17129E45) 
#define TN_ID_RENDEZVOUS        ((int)0x74289EBD) 
#define TN_ID_ALARM             ((int)0xFA5762BC)
#define TN_ID_CYCLIC            ((int)0xAB8F746B)
#define TN_ID_MESSAGEBUF				((int)0x9C9A6C89)

#define  TN_EXIT_AND_DELETE_TASK          1

//-- Task states
#define TSK_STATE_RUNNABLE            0x01
#define TSK_STATE_WAIT                0x04
#define TSK_STATE_SUSPEND             0x08
#define TSK_STATE_WAITSUSP            (TSK_STATE_SUSPEND | TSK_STATE_WAIT)
#define TSK_STATE_DORMANT             0x10

//--- Waiting
#define TSK_WAIT_REASON_SLEEP            0x0001
#define TSK_WAIT_REASON_SEM              0x0002
#define TSK_WAIT_REASON_EVENT            0x0004
#define TSK_WAIT_REASON_DQUE_WSEND       0x0008
#define TSK_WAIT_REASON_DQUE_WRECEIVE    0x0010
#define TSK_WAIT_REASON_MUTEX_C          0x0020          //-- ver 2.x
#define TSK_WAIT_REASON_MUTEX_C_BLK      0x0040          //-- ver 2.x
#define TSK_WAIT_REASON_MUTEX_I          0x0080          //-- ver 2.x
#define TSK_WAIT_REASON_MUTEX_H          0x0100          //-- ver 2.x
#define TSK_WAIT_REASON_RENDEZVOUS       0x0200          //-- ver 2.x
#define TSK_WAIT_REASON_MBF_WSEND        0x0400
#define TSK_WAIT_REASON_MBF_WRECEIVE     0x0800
#define TSK_WAIT_REASON_WFIXMEM          0x2000

#define TN_EVENT_ATTR_SINGLE            1
#define TN_EVENT_ATTR_MULTI             2
#define TN_EVENT_ATTR_CLR               4

#define TN_EVENT_WCOND_OR               8
#define TN_EVENT_WCOND_AND           0x10

#define TN_MUTEX_ATTR_INHERIT           0
#define TN_MUTEX_ATTR_CEILING					(1UL<<0)
#define TN_MUTEX_ATTR_RECURSIVE				(1UL<<1)

#define TN_CYCLIC_ATTR_START            1
#define TN_CYCLIC_ATTR_PHS              2

//-- Errors

#define TERR_TRUE                       1
#define TERR_NO_ERR                     0
#define TERR_OVERFLOW                 (-1) //-- OOV
#define TERR_WCONTEXT                 (-2) //-- Wrong context context error
#define TERR_WSTATE                   (-3) //-- Wrong state   state error
#define TERR_TIMEOUT                  (-4) //-- Polling failure or timeout
#define TERR_WRONG_PARAM              (-5)
#define TERR_UNDERFLOW                (-6)
#define TERR_OUT_OF_MEM               (-7)
#define TERR_ILUSE                    (-8) //-- Illegal using
#define TERR_NOEXS                    (-9) //-- Non-valid or Non-existent object
#define TERR_DLT                     (-10) //-- Waiting object deleted
#define NO_TIME_SLICE                   0
#define MAX_TIME_SLICE             0xFFFE

typedef void (*CBACK)(void *);

/* - Circular double-linked list queue - for internal using ------------------*/
typedef struct _CDLL_QUEUE {
	struct _CDLL_QUEUE *next;
	struct _CDLL_QUEUE *prev;
} CDLL_QUEUE;

typedef struct timer_event_block {
	CDLL_QUEUE queue; /* Timer event queue */
	unsigned long time; /* Event time */
	CBACK callback; /* Callback function */
	void *arg; /* Argument to be sent to callback function */
} TMEB;

/* - Task Control Block ------------------------------------------------------*/
typedef struct _TN_TCB {
	unsigned int *task_stk;        //-- Pointer to task's top of stack
	CDLL_QUEUE task_queue;  //-- Queue is used to include task in ready/wait lists
	CDLL_QUEUE *pwait_queue;  //-- Ptr to object's(semaphor,event,etc.) wait list,
														// that task has been included for waiting (ver 2.x)
	CDLL_QUEUE create_queue; //-- Queue is used to include task in create list only
#ifdef USE_MUTEXES
	CDLL_QUEUE mutex_queue;   //-- List of all mutexes that tack locked  (ver 2.x)
#endif
	TMEB wtmeb;
	unsigned int *stk_start;       //-- Base address of task's stack space
	int stk_size;         //-- Task's stack size (in sizeof(void*),not bytes)
	void *task_func_addr;  //-- filled on creation  (ver 2.x)
	void *task_func_param; //-- filled on creation  (ver 2.x)
	int base_priority;    //-- Task base priority  (ver 2.x)
	int priority;         //-- Task current priority
	int id_task;         //-- ID for verification(is it a task or another object?)
											 // All tasks have the same id_task magic number (ver 2.x)
	int task_state;       //-- Task state
	int task_wait_reason; //-- Reason for waiting
	int *wercd;    				//-- Waiting return code(reason why waiting  finished)
	WINFO winfo;					// Wait information
	int tslice_count;     //-- Time slice counter
	int wakeup_count;     //-- Wakeup request count - for statistic
	unsigned long time;             //-- Time work task
// Other implementation specific fields may be added below
} TN_TCB;

/* - Semaphore ---------------------------------------------------------------*/
typedef struct _TN_SEM {
	CDLL_QUEUE wait_queue;
	int count;
	int max_count;
	int id_sem;     //-- ID for verification(is it a semaphore or another object?)
									// All semaphores have the same id_sem magic number (ver 2.x)
} TN_SEM;

/* - Eventflag ---------------------------------------------------------------*/
typedef struct _TN_EVENT {
	CDLL_QUEUE wait_queue;
	int attr;       //-- Eventflag attribute
	unsigned int pattern;    //-- Initial value of the eventflag bit pattern
	int id_event;   //-- ID for verification(is it a event or another object?)
									// All events have the same id_event magic number (ver 2.x)
} TN_EVENT;

/* - Data queue --------------------------------------------------------------*/
typedef struct _TN_DQUE {
	CDLL_QUEUE wait_send_list;
	CDLL_QUEUE wait_receive_list;
	void **data_fifo;        //-- Array of void* to store data queue entries
	int num_entries;        //-- Capacity of data_fifo(num entries)
	int cnt;                // ���-�� ������ � �������
	int tail_cnt;           //-- Counter to processing data queue's Array of void*
	int header_cnt;         //-- Counter to processing data queue's Array of void*
	int id_dque;   //-- ID for verification(is it a data queue or another object?)
								 // All data queues have the same id_dque magic number (ver 2.x)
} TN_DQUE;

/* - Fixed-sized blocks memory pool ------------------------------------------*/
typedef struct _TN_FMP {
	CDLL_QUEUE wait_queue;
	unsigned int block_size;   //-- Actual block size (in bytes)
	int num_blocks;   //-- Capacity (Fixed-sized blocks actual max qty)
	void *start_addr;  //-- Memory pool actual start address
	void *free_list;   //-- Ptr to free block list
	int fblkcnt;      //-- Num of free blocks
	int id_fmp; //-- ID for verification(is it a fixed-sized blocks memory pool or another object?)
							// All Fixed-sized blocks memory pool have the same id_fmp magic number (ver 2.x)
} TN_FMP;

/* - Mutex -------------------------------------------------------------------*/
typedef struct _TN_MUTEX {
	CDLL_QUEUE wait_queue;       //-- List of tasks that wait a mutex
	CDLL_QUEUE mutex_queue; //-- To include in task's locked mutexes list (if any)
	int attr;             //-- Mutex creation attr - CEILING or INHERIT
	TN_TCB *holder;          //-- Current mutex owner(task that locked mutex)
	int ceil_priority;    //-- When mutex created with CEILING attr
	int cnt;              //-- Reserved
	int id_mutex;       //-- ID for verification(is it a mutex or another object?)
											// All mutexes have the same id_mutex magic number (ver 2.x)
} TN_MUTEX;

/* - Alarm Timer -------------------------------------------------------------*/
typedef struct _TN_ALARM {
	void *exinf; /* Extended information */
	void (*handler)(void *); /* Alarm handler address */
	unsigned int stat; /* Alarm handler state */
	TMEB tmeb; /* Timer event block */
	int id; /* ID for verification */
} TN_ALARM;

/* - Cyclic Timer ------------------------------------------------------------*/
typedef struct _TN_CYCLIC {
	void *exinf; /* Extended information */
	void (*handler)(void *); /* Cyclic handler address */
	unsigned int stat; /* Cyclic handler state */
	unsigned int attr; /* Cyclic handler attributes */
	unsigned long time; /* Cyclic time */
	TMEB tmeb; /* Timer event block */
	int id; /* ID for verification */
} TN_CYCLIC;

/* - Message Buffer ----------------------------------------------------------*/
typedef struct _TN_MBF {
	CDLL_QUEUE send_queue;	// Message buffer send wait queue
	CDLL_QUEUE recv_queue;	// Message buffer receive wait queue
	char *buf;    					// Message buffer address
	int msz;								// Length of message
	int num_entries;        // Capacity of data_fifo(num entries)
	int cnt;								// ���-�� ������ � �������
	int tail;     					// Next to the last message store address
	int head;     					// First message store address
	int id_mbf;							// Message buffer ID
} TN_MBF;

/* - User functions ----------------------------------------------------------*/
typedef void (*TN_USER_FUNC)(void);

typedef struct {
	TN_USER_FUNC app_init;
	unsigned long freq_timer;
} TN_OPTIONS;

/* - Global vars -------------------------------------------------------------*/
extern CDLL_QUEUE tn_create_queue; //-- all created tasks(now - for statictic only)
extern volatile int tn_created_tasks_qty;           //-- num of created tasks

extern volatile int tn_system_state; //-- System state -(running/not running,etc.)

extern TN_TCB * tn_curr_run_task;       //-- Task that  run now
extern TN_TCB * tn_next_task_to_run;    //-- Task to be run after switch context

extern volatile unsigned int tn_ready_to_run_bmp;
extern TN_USER_FUNC tn_app_init;

//-- Thanks to Vyacheslav Ovsiyenko - for his highly optimized code

#ifndef CONTAINING_RECORD
#define CONTAINING_RECORD(address, type, field)     \
        ((type *)((unsigned char *)(address) - (unsigned char *)(&((type *)0)->field)))
#endif

#define get_task_by_tsk_queue(que)                  \
        que ? CONTAINING_RECORD(que, TN_TCB, task_queue) : 0

#define get_mutex_by_mutex_queque(que)              \
        que ? CONTAINING_RECORD(que, TN_MUTEX, mutex_queue) : 0

#define get_mutex_by_wait_queque(que)               \
        que ? CONTAINING_RECORD(que, TN_MUTEX, wait_queue) : 0

#define get_task_by_block_queque(que)  \
        que ? CONTAINING_RECORD(que, TN_TCB, block_queue) : 0

#define get_mutex_by_lock_mutex_queque(que) \
        que ? CONTAINING_RECORD(que, TN_MUTEX, mutex_queue) : 0

#define get_timer_address(que) \
        que ? CONTAINING_RECORD(que, TMEB, queue) : 0

#define tn_time_after(a,b)      ((long)(b) - (long)(a) < 0)
#define tn_time_before(a,b)     tn_time_after(b,a)

#define tn_time_after_eq(a,b)   ((long)(a) - (long)(b) >= 0)
#define tn_time_before_eq(a,b)  tn_time_after_eq(b,a)

#ifdef __cplusplus
extern "C" {
#endif

/* - tn.c --------------------------------------------------------------------*/
void tn_start_system(TN_OPTIONS *opt);
int tn_sys_tslice_ticks(int priority, int value);

/* - tn_timer.c --------------------------------------------------------------*/
void tn_timer(void);
unsigned long tn_get_tick_count(void);
int tn_alarm_create(TN_ALARM *alarm,     // Alarm Control Block
	void (*handler)(void *),  // Alarm handler
	void *exinf      // Extended information
	);
int tn_alarm_delete(TN_ALARM *alarm);
int tn_alarm_start(TN_ALARM *alarm, unsigned long time);
int tn_alarm_stop(TN_ALARM *alarm);
int tn_cyclic_create(TN_CYCLIC *cyc, CBACK handler, void *exinf,
										 unsigned long cyctime, unsigned long cycphs,
										 unsigned int attr);
int tn_cyclic_delete(TN_CYCLIC *cyc);
int tn_cyclic_start(TN_CYCLIC *cyc);
int tn_cyclic_stop(TN_CYCLIC *cyc);

/* - tn_tasks.c --------------------------------------------------------------*/
int tn_task_create(TN_TCB *task,                   //-- task TCB
	void (*task_func)(void *param), //-- task function
	int priority,                   //-- task priority
	unsigned int *task_stack_start, //-- task stack first addr in memory (bottom)
	int task_stack_size,         //-- task stack size (in sizeof(void*),not bytes)
	void *param,                    //-- task function parameter
	int option                      //-- Creation option
	);
int tn_task_suspend(TN_TCB *task);
int tn_task_resume(TN_TCB *task);
int tn_task_sleep(unsigned long timeout);
unsigned long tn_task_time(TN_TCB *task);
int tn_task_wakeup(TN_TCB *task);
int tn_task_activate(TN_TCB *task);
int tn_task_release_wait(TN_TCB *task);
void tn_task_exit(int attr);
int tn_task_terminate(TN_TCB *task);
int tn_task_delete(TN_TCB *task);
int tn_task_change_priority(TN_TCB *task, int new_priority);

/* - tn_sem.c ----------------------------------------------------------------*/
int tn_sem_create(TN_SEM *sem, int start_value, int max_val);
int tn_sem_delete(TN_SEM *sem);
int tn_sem_signal(TN_SEM *sem);
int tn_sem_acquire(TN_SEM *sem, unsigned long timeout);

/* - tn_dqueue.c -------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*
 * �������� : tn_queue_create
 * �������� : ������� ������� ������. ���� id_dque ��������� TN_DQUE ��������������
 *						������ ���� ����������� � 0.
 * ���������: dque  - ��������� �� ������������ ��������� TN_DQUE.
 *						data_fifo - ��������� �� ������������ ������ void *.
 *												����� ���� ����� 0.
 *						num_entries - ������� �������. ����� ���� ����� 0, ����� ������
 *													�������� ����� ��� ������� � ���������� ������.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *----------------------------------------------------------------------------*/
int tn_queue_create(TN_DQUE *dque, void **data_fifo, int num_entries);

/*-----------------------------------------------------------------------------*
 * �������� : tn_queue_delete
 * �������� : ������� ������� ������.
 * ���������: dque  - ��������� �� ������������ ��������� TN_DQUE.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	������� �� ����������;
 *----------------------------------------------------------------------------*/
int tn_queue_delete(TN_DQUE *dque);

/*-----------------------------------------------------------------------------*
 * �������� : tn_queue_send
 * �������� : �������� ������ � ����� ������� �� ������������� �������� �������.
 * ���������: dque  - ���������� ������� ������.
 *						data_ptr  - ��������� �� ������.
 *						timeout - �����, � ������� �������� ������ ������ ���� ��������
 *											� �������.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	������� �� ����������;
 *							TERR_TIMEOUT	-	�������� �������� �������� �������;
 *----------------------------------------------------------------------------*/
int tn_queue_send(TN_DQUE *dque, void *data_ptr, unsigned long timeout);

/*-----------------------------------------------------------------------------*
 * �������� : tn_queue_send_first
 * �������� : �������� ������ � ������ ������� �� ������������� �������� �������.
 * ���������: dque  - ���������� ������� ������.
 *						data_ptr  - ��������� �� ������.
 *						timeout - �����, � ������� �������� ������ ������ ���� ��������
 *											� �������.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	������� �� ����������;
 *							TERR_TIMEOUT	-	�������� �������� �������� �������;
 *----------------------------------------------------------------------------*/
int tn_queue_send_first(TN_DQUE *dque, void *data_ptr, unsigned long timeout);

/*-----------------------------------------------------------------------------*
 * �������� : tn_queue_receive
 * �������� : ��������� ���� ������� ������ �� ������ ������� �� �������������
 * 						�������� �������.
 * ���������: dque  - ���������� ������� ������.
 * 						data_ptr  - ��������� �� �����, ���� ����� ������� ������.
 * 						timeout - �����, � ������� �������� ������ ������ ���� �������.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	������� �� ����������;
 *							TERR_TIMEOUT	-	�������� �������� �������� �������;
 *----------------------------------------------------------------------------*/
int tn_queue_receive(TN_DQUE *dque, void **data_ptr, unsigned long timeout);

/*-----------------------------------------------------------------------------*
 * �������� : tn_queue_flush
 * �������� : ���������� ������� ������.
 * ���������: dque  - ��������� �� ������� ������.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS  - ������� ������ �� ���� �������.
 *----------------------------------------------------------------------------*/
int tn_queue_flush(TN_DQUE *dque);

/*-----------------------------------------------------------------------------*
 * �������� : tn_queue_empty
 * �������� : ��������� ������� ������ �� �������.
 * ���������: dque  - ��������� �� ������� ������.
 * ���������: ���������� ���� �� ���������:
 *							TERR_TRUE - ������� ������ �����;
 *							TERR_NO_ERR - � ������� ������ ����;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS  - ������� ������ �� ���� �������.
 *----------------------------------------------------------------------------*/
int tn_queue_empty(TN_DQUE *dque);

/*-----------------------------------------------------------------------------*
 * �������� : tn_queue_full
 * �������� : ��������� ������� ������ �� ������ ����������.
 * ���������: dque  - ��������� �� ������� ������.
 * ���������: ���������� ���� �� ���������:
 *            	TERR_TRUE - ������� ������ ������;
 *              TERR_NO_ERR - ������� ������ �� ������;
 *              TERR_WRONG_PARAM  - ����������� ������ ���������;
 *              TERR_NOEXS  - ������� ������ �� ���� �������.
 *----------------------------------------------------------------------------*/
int tn_queue_full(TN_DQUE *dque);

/*-----------------------------------------------------------------------------*
 * �������� : tn_queue_cnt
 * �������� : ������� ���������� ���������� ��������� � ������� ������.
 * ���������: dque  - ���������� ������� ������.
 *						cnt - ��������� �� ������ ������, � ������� ����� ����������
 *									���������� ���������.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS  - ������� ������ �� ���� �������.
 *----------------------------------------------------------------------------*/
int tn_queue_cnt(TN_DQUE *dque, int *cnt);

/* - tn_message_buf.c --------------------------------------------------------*/

/*-----------------------------------------------------------------------------*
 * �������� : tn_mbf_create
 * �������� : ������� ����� ���������.
 * ���������: mbf	-	��������� �� ������������ ��������� TN_MBF.
 *						buf	- ��������� �� ���������� ��� ����� ��������� ������� ������.
 *									����� ���� ����� NULL.
 *						bufsz	- ������ ������ ��������� � ������. ����� ���� ����� 0,
 *										����� ������ �������� ����� ����� � ���������� ������.
 *						msz	-	������ ��������� � ������. ������ ���� ������ ����.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *----------------------------------------------------------------------------*/
extern int tn_mbf_create(TN_MBF *mbf, void *buf, int bufsz, int msz);

/*-----------------------------------------------------------------------------*
 * �������� : tn_mbf_delete
 * �������� : ������� ����� ���������.
 * ���������: mbf	- ��������� �� ������������ ��������� TN_MBF.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	����� ��������� �� ����������;
 *----------------------------------------------------------------------------*/
extern int tn_mbf_delete(TN_MBF *mbf);

/*-----------------------------------------------------------------------------*
 * �������� : tn_mbf_send
 * �������� : �������� ������ � ����� ������ ��������� �� �������������
 * 						�������� �������.
 * ���������: mbf	- ���������� ������ ���������.
 *						msg - ��������� �� ������.
 *						timeout - �����, � ������� �������� ������ ������ ���� ��������
 *											� �����.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	����� �� ����������;
 *							TERR_TIMEOUT	-	�������� �������� �������� �������;
 *----------------------------------------------------------------------------*/
extern int tn_mbf_send(TN_MBF *mbf, void *msg, unsigned long timeout);

/*-----------------------------------------------------------------------------*
 * �������� : tn_mbf_send_first
 * �������� : �������� ������ � ������ ������ ��������� �� �������������
 * 						�������� �������.
 * ���������: mbf	- ���������� ������ ���������.
 *						msg - ��������� �� ������.
 *						timeout - �����, � ������� �������� ������ ������ ���� ��������
 *											� �����.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	����� �� ����������;
 *							TERR_TIMEOUT	-	�������� �������� �������� �������;
 *----------------------------------------------------------------------------*/
extern int tn_mbf_send_first(TN_MBF *mbf, void *msg, unsigned long timeout);

/*-----------------------------------------------------------------------------*
 * �������� : tn_mbf_receive
 * �������� : ��������� ���� ������� ������ �� ������ ������ ���������
 * 						�� ������������� �������� �������.
 * ���������: mbf	- ���������� ������ ���������.
 * 						msg - ��������� �� �����, ���� ����� ������� ������.
 * 						timeout - �����, � ������� �������� ������ ������ ���� �������.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	����� �� ����������;
 *							TERR_TIMEOUT	-	�������� �������� �������� �������;
 *----------------------------------------------------------------------------*/
extern int tn_mbf_receive(TN_MBF *mbf, void *msg, unsigned long timeout);

/*-----------------------------------------------------------------------------*
 * �������� : tn_mbf_flush
 * �������� : ���������� ����� ���������.
 * ���������: mbf	- ���������� ������ ���������.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS  - ����� �� ��� ������.
 *----------------------------------------------------------------------------*/
extern int tn_mbf_flush(TN_MBF *mbf);

/*-----------------------------------------------------------------------------*
 * �������� : tn_mbf_empty
 * �������� : ��������� ����� ��������� �� �������.
 * ���������: mbf	- ���������� ������ ���������.
 * ���������: ���������� ���� �� ���������:
 *							TERR_TRUE - ����� ��������� ����;
 *							TERR_NO_ERR - � ������ ������ ����;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS  - ����� �� ��� ������.
 *----------------------------------------------------------------------------*/
extern int tn_mbf_empty(TN_MBF *mbf);

/*-----------------------------------------------------------------------------*
 * �������� : tn_mbf_full
 * �������� : ��������� ����� ��������� �� ������ ����������.
 * ���������: mbf	- ���������� ������ ���������.
 * ���������: ���������� ���� �� ���������:
 *            	TERR_TRUE - ����� ��������� ������;
 *              TERR_NO_ERR - ����� ��������� �� ������;
 *              TERR_WRONG_PARAM  - ����������� ������ ���������;
 *              TERR_NOEXS  - ����� ��������� �� ��� ������.
 *----------------------------------------------------------------------------*/
extern int tn_mbf_full(TN_MBF *mbf);

/*-----------------------------------------------------------------------------*
 * �������� : tn_mbf_cnt
 * �������� : ������� ���������� ���������� ��������� � ������ ���������.
 * ���������: mbf	- ���������� ������ ���������.
 *						cnt - ��������� �� ������ ������, � ������� ����� ����������
 *									���������� ���������.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS  - ����� ��������� �� ��� ������.
 *----------------------------------------------------------------------------*/
extern int tn_mbf_cnt(TN_MBF *mbf, int *cnt);

/* - tn_event.c --------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*
 * �������� : tn_event_create
 * �������� : ������� ���� �������.
 * ���������: evf	-	��������� �� ���������������� ��������� TN_EVENT
 * 						attr	-	�������� ������������ �����.
 * 										�������� ��������� ��������� �����������:
 * 										TN_EVENT_ATTR_CLR	-	��������� �������������� ������� �����
 * 																				����� ��� ���������. ��������
 * 																				���������� ������ ��������� � ���������
 * 																				TN_EVENT_ATTR_SINGLE.
 * 										TN_EVENT_ATTR_SINGLE	- ������������� ����� ������ � �����
 * 																						������. ������������� � ����������
 * 																						������� �� �����������.
 * 										TN_EVENT_ATTR_MULTI	-	������������� ����� �������� �
 * 																					���������� �������.
 * 										�������� TN_EVENT_ATTR_SINGLE � TN_EVENT_ATTR_MULTI
 * 										������� �����������. �� ����������� ������������ ��
 * 										������������, �� ����� �� ����������� ������ �� ���������
 * 										�� ���� �� ���� ���������.
 * 						pattern	- ��������� ������� ���� �� �������� ���� �����������
 * 											��������� �����. ������ ������ ���� ����� 0.
 * ���������: ���������� TERR_NO_ERR ���� ��������� ��� ������, � ���������
 * 						������ TERR_WRONG_PARAM
 *----------------------------------------------------------------------------*/
extern int tn_event_create(TN_EVENT *evf, int attr, unsigned int pattern);

/*-----------------------------------------------------------------------------*
 * �������� : tn_event_delete
 * �������� : ������� ���� �������.
 * ���������: evf	- ��������� �� ������������ ��������� TN_EVENT.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	���� ������� �� ����������;
 *----------------------------------------------------------------------------*/
extern int tn_event_delete(TN_EVENT *evf);

/*-----------------------------------------------------------------------------*
 * �������� : tn_event_wait
 * �������� : ������� ��������� ����� ������� � ������� ��������� ���������
 * 						�������.
 * ���������: evf	- ��������� �� ������������ ��������� TN_EVENT.
 * 						wait_pattern	- ��������� ���������� �����. �� ����� ���� ����� 0.
 * 						wait_mode	- ����� ��������.
 * 												�������� ���� �� �����������:
 * 													TN_EVENT_WCOND_OR	- ��������� ��������� ������ ����
 * 																							�� ���������.
 * 													TN_EVENT_WCOND_AND	-	��������� ��������� ���� �����
 * 																								�� ���������.
 * 						p_flags_pattern	-	��������� �� ����������, � ������� ����� ��������
 * 															�������� ���������� ����� �� ��������� ��������.
 * 						timeout	-	����� �������� ��������� ������ �������.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	���� ������� �� ����������;
 *							TERR_ILUSE	-	���� ��� ������ � ��������� TN_EVENT_ATTR_SINGLE,
 *														� ��� �������� ������������ ����� ����� ������.
 *							TERR_TIMEOUT	-	����� �������� �������.
 *----------------------------------------------------------------------------*/
extern int tn_event_wait(TN_EVENT *evf, unsigned int wait_pattern,
												 int wait_mode, unsigned int *p_flags_pattern,
												 unsigned long timeout);

/*-----------------------------------------------------------------------------*
 * �������� : tn_event_set
 * �������� : ������������� ����� �������.
 * ���������: evf	- ��������� �� ������������ ��������� TN_EVENT.
 * 						pattern	-	���������� �����. 1 � ������� �������������
 * 											���������������� �����. �� ����� ���� ����� 0.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	���� ������� �� ����������;
 *----------------------------------------------------------------------------*/
extern int tn_event_set(TN_EVENT *evf, unsigned int pattern);

/*-----------------------------------------------------------------------------*
 * �������� : tn_event_clear
 * �������� : ������� ���� �������.
 * ���������: evf	- ��������� �� ������������ ��������� TN_EVENT.
 * 						pattern	-	���������� �����. 1 � ������� �������������
 * 											���������� �����. �� ����� ���� ����� 0.
 * ���������: ���������� ���� �� ���������:
 *							TERR_NO_ERR - ������� ��������� ��� ������;
 *							TERR_WRONG_PARAM  - ����������� ������ ���������;
 *							TERR_NOEXS	-	���� ������� �� ����������;
 *----------------------------------------------------------------------------*/
extern int tn_event_clear(TN_EVENT *evf, unsigned int pattern);

/* - tn_mem.c ----------------------------------------------------------------*/
int tn_fmem_create(TN_FMP *fmp, void *start_addr, unsigned int block_size,
									 int num_blocks);
int tn_fmem_delete(TN_FMP *fmp);
int tn_fmem_get(TN_FMP *fmp, void **p_data, unsigned long timeout);
int tn_fmem_release(TN_FMP *fmp, void *p_data);

/* - tn_mutex.c --------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*
 * �������� : tn_mutex_create
 * �������� :	������� �������
 * ���������:	mutex	-	��������� �� ���������������� ��������� TN_MUTEX
 * 										(���������� ��������)
 * 						attribute	-	�������� ������������ ��������.
 *												�������� ��������� ��������� ��������:
 *												TN_MUTEX_ATTR_CEILING	-	������������ ��������
 *																								���������� ���������� ���
 *																								���������� �������� ����������
 *																								� �������� ����������.
 *												TN_MUTEX_ATTR_INHERIT	-	������������ ��������
 *																								������������ ���������� ���
 *																								���������� �������� ����������.
 *												TN_MUTEX_ATTR_RECURSIVE	-	�������� ����������� ������
 *																									��������, �������, �������
 *																									��� ��������� �������.
 *						ceil_priority	-	������������ ��������� �� ���� �����, �������
 *														����� ������� ��������. �������� ������������,
 *														���� attribute = TN_MUTEX_ATTR_INHERIT
 * ���������: ���������� TERR_NO_ERR ���� ��������� ��� ������, � ���������
 * 						������ TERR_WRONG_PARAM
 *----------------------------------------------------------------------------*/
int tn_mutex_create(TN_MUTEX *mutex, int attribute, int ceil_priority);
int tn_mutex_delete(TN_MUTEX *mutex);
int tn_mutex_lock(TN_MUTEX *mutex, unsigned long timeout);
int tn_mutex_unlock(TN_MUTEX *mutex);

/* - tn_delay.c --------------------------------------------------------------*/
void tn_mdelay(unsigned long ms);
void tn_udelay(unsigned long usecs);

/* - tn_sprintf.c ------------------------------------------------------------*/
extern int tn_snprintf(char *outStr, int maxLen, const char *fmt, ...) __attribute__((nonnull(1,3)));
extern int tn_abs(int i);
extern int tn_strlen(const char *str) __attribute__((nonnull));
extern char* tn_strcpy(char *dst, const char *src) __attribute__((nonnull));
extern char* tn_strncpy(char *dst, const char *src, int n) __attribute__((nonnull));
extern char* tn_strcat(char *dst, const char *src) __attribute__((nonnull));
extern char* tn_strncat(char *dst, const char *src, int n) __attribute__((nonnull));
extern int tn_strcmp(const char *str1, const char *str2) __attribute__((nonnull));
extern int tn_strncmp(const char *str1, const char *str2, int num) __attribute__((nonnull));
extern void* tn_memset(void *dst, int ch, int length) __attribute__((nonnull));
extern void* tn_memcpy(void *s1, const void *s2, int n) __attribute__((nonnull));
extern int tn_memcmp(const void *s1, const void *s2, int n) __attribute__((nonnull));
extern int tn_atoi(const char *s) __attribute__((nonnull));

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

/*------------------------------ ����� ����� ---------------------------------*/
