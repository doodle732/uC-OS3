/*
*********************************************************************************************************
*                                              uC/OS-III
*                                        The Real-Time Kernel
*
*                    Copyright 2009-2022 Silicon Laboratories Inc. www.silabs.com
*
*                                 SPDX-License-Identifier: APACHE-2.0
*
*               This software is subject to an open source license and is distributed by
*                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
*                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                           Generic ARM Port
*
* File      : os_cpu_c.c
* Version   : V3.08.02
*********************************************************************************************************
* For       : ARM7 or ARM9
* Mode      : ARM  or Thumb
* Toolchain : IAR EWARM V5.xx and higher
*********************************************************************************************************
*/

#define   OS_CPU_GLOBALS

#ifdef VSC_INCLUDE_SOURCE_FILE_NAMES
const  CPU_CHAR  *os_cpu_c__c = "$Id: $";
#endif

#ifdef __cplusplus
extern  "C" {
#endif

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  "../../../../Source/os.h"

/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*
* Note(s) : 1) ARM_MODE_ARM       is the CPSR bit mask for ARM Mode
*           2) ARM_MODE_THUMB     is the CPSR bit mask for THUMB Mode
*           3) ARM_SVC_MODE_THUMB is the CPSR bit mask for SVC MODE + THUMB Mode
*           4) ARM_SVC_MODE_ARM   is the CPSR bit mask for SVC MODE + ARM Mode
*********************************************************************************************************
*/

#define  ARM_MODE_ARM           0x00000000u
#define  ARM_MODE_THUMB         0x00000020u

#define  ARM_SVC_MODE_THUMB    (0x00000013u + ARM_MODE_THUMB)
#define  ARM_SVC_MODE_ARM      (0x00000013u + ARM_MODE_ARM)


/*
*********************************************************************************************************
*                                           IDLE TASK HOOK
*
* Description: This function is called by the idle task.  This hook has been added to allow you to do
*              such things as STOP the CPU to conserve power.
*
* Arguments  : None.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSIdleTaskHook (void)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppIdleTaskHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppIdleTaskHookPtr)();
    }
#endif
}


/*
*********************************************************************************************************
*                                       OS INITIALIZATION HOOK
*
* Description: This function is called by OSInit() at the beginning of OSInit().
*
* Arguments  : None.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSInitHook (void)
{
    CPU_STK_SIZE   i;
    CPU_STK       *p_stk;


    p_stk = OSCfg_ISRStkBasePtr;                            /* Clear the ISR stack                                    */
    for (i = 0u; i < OSCfg_ISRStkSize; i++) {
        *p_stk++ = (CPU_STK)0u;
    }
    OS_CPU_ExceptStkBase = (CPU_STK *)(OSCfg_ISRStkBasePtr + OSCfg_ISRStkSize - 1u);
}


/*
*********************************************************************************************************
*                                         STATISTIC TASK HOOK
*
* Description: This function is called every second by uC/OS-III's statistics task.  This allows your
*              application to add functionality to the statistics task.
*
* Arguments  : None.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSStatTaskHook (void)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppStatTaskHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppStatTaskHookPtr)();
    }
#endif
}


/*
*********************************************************************************************************
*                                          TASK CREATION HOOK
*
* Description: This function is called when a task is created.
*
* Arguments  : p_tcb        Pointer to the task control block of the task being created.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSTaskCreateHook (OS_TCB  *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskCreateHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskCreateHookPtr)(p_tcb);
    }
#else
    (void)p_tcb;                                            /* Prevent compiler warning                               */
#endif
}


/*
*********************************************************************************************************
*                                           TASK DELETION HOOK
*
* Description: This function is called when a task is deleted.
*
* Arguments  : p_tcb        Pointer to the task control block of the task being deleted.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSTaskDelHook (OS_TCB  *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskDelHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskDelHookPtr)(p_tcb);
    }
#else
    (void)p_tcb;                                            /* Prevent compiler warning                               */
#endif
}


/*
*********************************************************************************************************
*                                            TASK RETURN HOOK
*
* Description: This function is called if a task accidentally returns.  In other words, a task should
*              either be an infinite loop or delete itself when done.
*
* Arguments  : p_tcb        Pointer to the task control block of the task that is returning.
*
* Note(s)    : None.
*********************************************************************************************************
*/

void  OSTaskReturnHook (OS_TCB  *p_tcb)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskReturnHookPtr != (OS_APP_HOOK_TCB)0) {
        (*OS_AppTaskReturnHookPtr)(p_tcb);
    }
#else
    (void)p_tcb;                                            /* Prevent compiler warning                               */
#endif
}


/*
**********************************************************************************************************
*                                       INITIALIZE A TASK'S STACK
*
* Description: This function is called by OS_Task_Create() or OSTaskCreateExt() to initialize the stack
*              frame of the task being created. This function is highly processor specific.
*
* Arguments  : p_task       Pointer to the task entry point address.
*
*              p_arg        Pointer to a user supplied data area that will be passed to the task
*                               when the task first executes.
*
*              p_stk_base   Pointer to the base address of the stack.
*
*              stk_size     Size of the stack, in number of CPU_STK elements.
*
*              opt          Options used to alter the behavior of OS_Task_StkInit().
*                            (see OS.H for OS_TASK_OPT_xxx).
*
* Returns    : Always returns the location of the new top-of-stack' once the processor registers have
*              been placed on the stack in the proper order.
*
* Note(s)    : 1) Interrupts are enabled when task starts executing.
*
*              2) All tasks run in SVC mode.
**********************************************************************************************************
*/

CPU_STK  *OSTaskStkInit (OS_TASK_PTR    p_task,
                         void          *p_arg,
                         CPU_STK       *p_stk_base,
                         CPU_STK       *p_stk_limit,
                         CPU_STK_SIZE   stk_size,
                         OS_OPT         opt)
{
    CPU_STK  *p_stk;
    CPU_STK   task_addr;


    (void)&p_stk_limit;                                     /* Prevent compiler warning                               */
    (void)&opt;

    p_stk     = &p_stk_base[stk_size];                      /* Load stack pointer                                     */
    task_addr = (CPU_STK)p_task & ~1u;                      /* Mask off lower bit in case task is thumb mode          */

    *--p_stk  = (CPU_STK)task_addr;                         /* Entry Point                                            */
    *--p_stk  = (CPU_STK)OS_TaskReturn;                     /* R14 (LR)                                               */
    *--p_stk  = (CPU_STK)0x12121212u;                       /* R12                                                    */
    *--p_stk  = (CPU_STK)0x11111111u;                       /* R11                                                    */
    *--p_stk  = (CPU_STK)0x10101010u;                       /* R10                                                    */
    *--p_stk  = (CPU_STK)0x09090909u;                       /* R9                                                     */
    *--p_stk  = (CPU_STK)0x08080808u;                       /* R8                                                     */
    *--p_stk  = (CPU_STK)0x07070707u;                       /* R7                                                     */
    *--p_stk  = (CPU_STK)0x06060606u;                       /* R6                                                     */
    *--p_stk  = (CPU_STK)0x05050505u;                       /* R5                                                     */
    *--p_stk  = (CPU_STK)0x04040404u;                       /* R4                                                     */
    *--p_stk  = (CPU_STK)0x03030303u;                       /* R3                                                     */
    *--p_stk  = (CPU_STK)0x02020202u;                       /* R2                                                     */
    *--p_stk  = (CPU_STK)0x01010101u;                       /* R1                                                     */
    *--p_stk  = (CPU_STK)p_arg;                             /* R0 : argument                                          */

    if (((CPU_STK)p_task & 0x01u) == 0x01u) {               /* See if task runs in Thumb or ARM mode                  */
       *--p_stk = (CPU_STK)ARM_SVC_MODE_THUMB;              /* CPSR  (Enable IRQ and FIQ interrupts, THUMB-mode)      */
    } else {
       *--p_stk = (CPU_STK)ARM_SVC_MODE_ARM;                /* CPSR  (Enable IRQ and FIQ interrupts, ARM-mode)        */
    }

    return (p_stk);
}


/*
*********************************************************************************************************
*                                           TASK SWITCH HOOK
*
* Description: This function is called when a task switch is performed.  This allows you to perform other
*              operations during a context switch.
*
* Arguments  : None.
*
* Note(s)    : 1) Interrupts are disabled during this call.
*              2) It is assumed that the global pointer 'OSTCBHighRdyPtr' points to the TCB of the task
*                 that will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCurPtr' points
*                 to the task being switched out (i.e. the preempted task).
*********************************************************************************************************
*/

void  OSTaskSwHook (void)
{
#if OS_CFG_TASK_PROFILE_EN > 0u
    CPU_TS  ts;
#endif
#ifdef  CPU_CFG_INT_DIS_MEAS_EN
    CPU_TS  int_dis_time;
#endif



#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTaskSwHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppTaskSwHookPtr)();
    }
#endif

#if OS_CFG_TASK_PROFILE_EN > 0u
    ts = OS_TS_GET();
    if (OSTCBCurPtr != OSTCBHighRdyPtr) {
        OSTCBCurPtr->CyclesDelta  = ts - OSTCBCurPtr->CyclesStart;
        OSTCBCurPtr->CyclesTotal += (OS_CYCLES)OSTCBCurPtr->CyclesDelta;
    }

    OSTCBHighRdyPtr->CyclesStart = ts;
#endif

#ifdef  CPU_CFG_INT_DIS_MEAS_EN
    int_dis_time = CPU_IntDisMeasMaxCurReset();             /* Keep track of per-task interrupt disable time          */
    if (OSTCBCurPtr->IntDisTimeMax < int_dis_time) {
        OSTCBCurPtr->IntDisTimeMax = int_dis_time;
    }
#endif

#if OS_CFG_SCHED_LOCK_TIME_MEAS_EN > 0u
                                                            /* Keep track of per-task scheduler lock time             */
    if (OSTCBCurPtr->SchedLockTimeMax < OSSchedLockTimeMaxCur) {
        OSTCBCurPtr->SchedLockTimeMax = OSSchedLockTimeMaxCur;
    }
    OSSchedLockTimeMaxCur = (CPU_TS)0;                      /* Reset the per-task value                               */
#endif
}


/*
*********************************************************************************************************
*                                              TICK HOOK
*
* Description: This function is called every tick.
*
* Arguments  : None.
*
* Note(s)    : 1) This function is assumed to be called from the Tick ISR.
*********************************************************************************************************
*/

void  OSTimeTickHook (void)
{
#if OS_CFG_APP_HOOKS_EN > 0u
    if (OS_AppTimeTickHookPtr != (OS_APP_HOOK_VOID)0) {
        (*OS_AppTimeTickHookPtr)();
    }
#endif
}


/*
*********************************************************************************************************
*                                    INITIALIZE EXCEPTION VECTORS
*
* Description: This function initialize exception vectors to the default handlers.
*
* Arguments  : None.
*********************************************************************************************************
*/

void  OS_CPU_InitExceptVect (void)
{
    (*(CPU_ADDR *)OS_CPU_ARM_EXCEPT_UNDEF_INSTR_VECT_ADDR)       =           OS_CPU_ARM_INSTR_JUMP_TO_HANDLER;
    (*(CPU_ADDR *)OS_CPU_ARM_EXCEPT_UNDEF_INSTR_HANDLER_ADDR)    = (CPU_ADDR)OS_CPU_ARM_ExceptUndefInstrHndlr;

    (*(CPU_ADDR *)OS_CPU_ARM_EXCEPT_SWI_VECT_ADDR)               =           OS_CPU_ARM_INSTR_JUMP_TO_HANDLER;
    (*(CPU_ADDR *)OS_CPU_ARM_EXCEPT_SWI_HANDLER_ADDR)            = (CPU_ADDR)OS_CPU_ARM_ExceptSwiHndlr;

    (*(CPU_ADDR *)OS_CPU_ARM_EXCEPT_PREFETCH_ABORT_VECT_ADDR)    =           OS_CPU_ARM_INSTR_JUMP_TO_HANDLER;
    (*(CPU_ADDR *)OS_CPU_ARM_EXCEPT_PREFETCH_ABORT_HANDLER_ADDR) = (CPU_ADDR)OS_CPU_ARM_ExceptPrefetchAbortHndlr;

    (*(CPU_ADDR *)OS_CPU_ARM_EXCEPT_DATA_ABORT_VECT_ADDR)        =           OS_CPU_ARM_INSTR_JUMP_TO_HANDLER;
    (*(CPU_ADDR *)OS_CPU_ARM_EXCEPT_DATA_ABORT_HANDLER_ADDR)     = (CPU_ADDR)OS_CPU_ARM_ExceptDataAbortHndlr;

    (*(CPU_ADDR *)OS_CPU_ARM_EXCEPT_IRQ_VECT_ADDR)               =           OS_CPU_ARM_INSTR_JUMP_TO_HANDLER;
    (*(CPU_ADDR *)OS_CPU_ARM_EXCEPT_IRQ_HANDLER_ADDR)            = (CPU_ADDR)OS_CPU_ARM_ExceptIrqHndlr;

    (*(CPU_ADDR *)OS_CPU_ARM_EXCEPT_FIQ_VECT_ADDR)               =           OS_CPU_ARM_INSTR_JUMP_TO_HANDLER;
    (*(CPU_ADDR *)OS_CPU_ARM_EXCEPT_FIQ_HANDLER_ADDR)            = (CPU_ADDR)OS_CPU_ARM_ExceptFiqHndlr;
}

#ifdef __cplusplus
}
#endif
