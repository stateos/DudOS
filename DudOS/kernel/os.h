/******************************************************************************

    @file    DudOS: os.h
    @author  Rajmund Szymanski
    @date    26.07.2018
    @brief   This file provides set of functions for DudOS.

 ******************************************************************************

   Copyright (c) 2018 Rajmund Szymanski. All rights reserved.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to
   deal in the Software without restriction, including without limitation the
   rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
   sell copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   IN THE SOFTWARE.

 ******************************************************************************/

#ifndef __DUDOS

#define __DUDOS_MAJOR        0
#define __DUDOS_MINOR        1
#define __DUDOS_BUILD        0

#define __DUDOS        ((((__DUDOS_MAJOR)&0xFFUL)<<24)|(((__DUDOS_MINOR)&0xFFUL)<<16)|((__DUDOS_BUILD)&0xFFFFUL))

#define __DUDOS__           "DudOS v" STRINGIZE(__DUDOS_MAJOR) "." STRINGIZE(__DUDOS_MINOR) "." STRINGIZE(__DUDOS_BUILD)

#define STRINGIZE(n) STRINGIZE_HELPER(n)
#define STRINGIZE_HELPER(n) #n

/* -------------------------------------------------------------------------- */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "osport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* System =================================================================== */

typedef void fun_t( void );

/* Public ------------------------------------------------------------------- */
void    sys_init ( void );           // port function - initialize the system timer
cnt_t   sys_time ( void );           // port function - get current value of the system timer (in milliseconds)
void    sys_start( void );           // system function - start the system scheduler
/* -------------------------------------------------------------------------- */
#define MSEC                  1      // time multiplier given in milliseconds
#define SEC                1000      // time multiplier given in seconds
#define MIN               60000      // time multiplier given in minutes
#define INFINITE           ( ~0UL )  // time value for infinite wait

/* Task ( goto / label ) ==================================================== */

#ifdef  USE_GOTO

typedef struct __tsk { cnt_t start; cnt_t delay; void *state; fun_t *function; struct __tsk *next; } tsk_t;

/* Private ------------------------------------------------------------------ */
#define TSK_CONCATENATE(LINE, line)     LINE ## line
#define TSK_LABEL(line)                 TSK_CONCATENATE(LINE, line)
#define TSK_STATE(line)              && TSK_LABEL(line)
#define TSK_BEGIN()                     if (Current->state) goto *Current->state
#define TSK_END()

/* Task ( switch / case ) =================================================== */

#else//!USE_GOTO

typedef struct __tsk { cnt_t start; cnt_t delay; intptr_t state; fun_t *function; struct __tsk *next; } tsk_t;

/* Private ------------------------------------------------------------------ */
#define TSK_LABEL(line)                 /* falls through */ case line
#define TSK_STATE(line)                 line
#define TSK_BEGIN()                     switch (Current->state) { case 0:
#define TSK_END()                       default: break; }

#endif//USE_GOTO

/* -------------------------------------------------------------------------- */
#define TSK_SET()                  do { Current->state = TSK_STATE(__LINE__);         } while(0); TSK_LABEL(__LINE__):
#define TSK_YIELD()                do { Current->state = TSK_STATE(__LINE__); return; } while(0); TSK_LABEL(__LINE__):
#define TSK_INIT(tsk)              do { (tsk)->state = 0;                             } while(0)
#define TSK_FINI(tsk)              do { (tsk)->function = 0;                          } while(0)
#define TSK_CALL(tsk)              do { if ((tsk)->function) (tsk)->function();       } while(0)

/* Public ------------------------------------------------------------------- */
#define  OS_TSK(tsk, fun)               tsk_t tsk[1] = { { 0, 0, 0, fun, 0 } }

#define  OS_TSK_DEF(tsk)                void tsk ## __fun( void ); \
                                        OS_TSK(tsk, tsk ## __fun); \
                                        void tsk ## __fun( void )

#define  OS_TSK_START(tsk)              void tsk ## __fun( void ); \
                                        OS_TSK(tsk, tsk ## __fun); \
           __attribute__((constructor)) void tsk ## __run( void ) { tsk_start(tsk); } \
                                        void tsk ## __fun( void )
/* -------------------------------------------------------------------------- */
bool    tsk_start( tsk_t *tsk );     // system function - add task to queue
tsk_t * tsk_this ( void );           // system function - return current task
/* -------------------------------------------------------------------------- */
//      tsk_begin                       necessary task prologue
#define tsk_begin()                     tsk_t * const Current = tsk_this(); TSK_BEGIN();                 do {} while(0)
//      tsk_end                         necessary task epilogue
#define tsk_end()                       TSK_END();                                                       do {} while(0)
//      tsk_waitWhile                   wait while the condition (cnd) is true
#define tsk_waitWhile(cnd)         do { TSK_SET(); if (cnd) return; TSK_INIT(Current);                       } while(0)
//      tsk_waitUntil                   wait while the condition (cnd) is false
#define tsk_waitUntil(cnd)         do { tsk_waitWhile(!(cnd));                                               } while(0)
//      tsk_join                        wait while the task (tsk) is working
#define tsk_join(tsk)              do { tsk_waitWhile((tsk)->function);                                      } while(0)
//      tsk_startFrom                   start new or restart previously stopped task (tsk) with function (fun)
#define tsk_startFrom(tsk, fun)    do { if (tsk_start(tsk) || (tsk)->function == 0) (tsk)->function = (fun); } while(0)
//      tsk_exit                        restart the current task from the initial state
#define tsk_exit()                 do { return;                                                              } while(0)
//      tsk_stop                        stop the current task; it will no longer be executed
#define tsk_stop()                 do { TSK_FINI(Current); return;                                           } while(0)
//      tsk_kill                        stop the task (tsk); it will no longer be executed
#define tsk_kill(tsk)              do { TSK_FINI(tsk); if ((tsk) == Current) return; TSK_INIT(tsk);          } while(0)
//      tsk_yield                       pass control to the next ready task
#define tsk_yield()                do { TSK_YIELD(); TSK_INIT(Current);                                      } while(0)
//      tsk_flip                        restart the current task with function (fun)
#define tsk_flip(fun)              do { Current->function = (fun); return;                                   } while(0)
//      tsk_sleepFor                    delay the current task for the duration of (dly)
#define tsk_sleepFor(dly)          do { tmr_startFor(Current, dly); tmr_wait(Current);                       } while(0)
//      tsk_sleepUntil                  delay the current task until time "tim"
#define tsk_sleepUntil(tim)        do { tsk_sleepFor((tim) - Current->start);                                } while(0)
//      tsk_sleep                       delay the current task indefinitely
#define tsk_sleep()                do { tsk_sleepFor(INFINITE);                                              } while(0)
//      tsk_delay                       delay the current task for the duration of (dly)
#define tsk_delay(dly)             do { tsk_sleepFor(dly);                                                   } while(0)
//      tsk_suspend                     suspend the current task
#define tsk_suspend()              do { tsk_sleep();                                                         } while(0)
//      tsk_resume                      resume the task (tsk)
#define tsk_resume(tsk)            do { tmr_stop(tsk);                                                       } while(0)

/* Timer ==================================================================== */

typedef struct __tmr { cnt_t start; cnt_t delay; } tmr_t;

/* Private ------------------------------------------------------------------ */
#define TMR_INIT(tmr, dly)         do { (tmr)->start = sys_time(), (tmr)->delay = (dly); } while(0)
#define TMR_WAIT(tmr)                 ( sys_time() - (tmr)->start + 1 <= (tmr)->delay )

/* Public ------------------------------------------------------------------- */
#define  OS_TMR(tmr)                    tmr_t tmr[1] = { { 0, 0   } }
#define  OS_TMR_START(tmr, dly)         tmr_t tmr[1] = { { 0, dly } }
/* -------------------------------------------------------------------------- */
//      tmr_startFor                    start the timer (tmr) for the duration of (dly)
#define tmr_startFor(tmr, dly)     do { TMR_INIT(tmr, dly);                  } while(0)
//      tmr_startUntil                  start the timer (tmr) until the time (tim)
#define tmr_startUntil(tmr, tim)   do { TMR_INIT(tmr, (tim) - (tmr)->start); } while(0)
//      tmr_stop                        stop the timer (tmr)
#define tmr_stop(tmr)              do { TMR_INIT(tmr, 0);                    } while(0)
//      tmr_wait                        wait while the timer (tmr) is counting down
#define tmr_wait(tmr)              do { tsk_waitWhile(TMR_WAIT(tmr));        } while(0)
//      tmr_expired                     check if the timer has finished counting down
#define tmr_expired(tmr)              ( TMR_WAIT(tmr) == false )

/* Binary semaphore ========================================================= */

typedef struct __sem { uint_fast8_t state; } sem_t;

/* Private ------------------------------------------------------------------ */
#define SEM_CLR(sem)               do { (sem)->state = 0; } while(0)
#define SEM_SET(sem)               do { (sem)->state = 1; } while(0)
#define SEM_GET(sem)                  ( (sem)->state )

/* Public ------------------------------------------------------------------- */
#define  OS_SEM(sem, ini)               sem_t sem[1] = { { ini } }
/* -------------------------------------------------------------------------- */
//      sem_wait                        wait for the semaphore (sem)
#define sem_wait(sem)              do { tsk_waitUntil(SEM_GET(sem)); SEM_CLR(sem); } while(0)
//      sem_give                        release the semaphore (sem)
#define sem_give(sem)              do { SEM_SET(sem);                              } while(0)

/* Protected signal ========================================================= */

typedef struct __sig { uint_fast8_t signal; } sig_t;

/* Private ------------------------------------------------------------------ */
#define SIG_CLR(sig)               do { (sig)->signal = 0; } while(0)
#define SIG_SET(sig)               do { (sig)->signal = 1; } while(0)
#define SIG_GET(sig)                  ( (sig)->signal )

/* Public ------------------------------------------------------------------- */
#define  OS_SIG(sig)                    sig_t sig[1] = { { 0 } }
/* -------------------------------------------------------------------------- */
//      sig_wait                        wait for the signal (sig)
#define sig_wait(sig)              do { tsk_waitUntil(SIG_GET(sig)); } while(0)
//      sig_give                        release the signal (sig)
#define sig_give(sig)              do { SIG_SET(sig);                } while(0)
//      sig_clear                       reset the signal (sig)
#define sig_clear(sig)             do { SIG_CLR(sig);                } while(0)

/* Event ==================================================================== */

typedef struct __evt { uintptr_t event; } evt_t;

/* Private ------------------------------------------------------------------ */
#define EVT_CLR(evt)               do { (evt)->event = 0;     } while(0)
#define EVT_PUT(evt, val)          do { (evt)->event = (val); } while(0)
#define EVT_GET(evt)                  ( (evt)->event )

/* Public ------------------------------------------------------------------- */
#define  OS_EVT(evt)                    evt_t evt[1] = { { 0 } }
/* -------------------------------------------------------------------------- */
//      evt_wait                        wait for a the new value of the event (evt)
#define evt_wait(evt)              do { EVT_CLR(evt); tsk_waitUntil(EVT_GET(evt)); } while(0)
//      evt_take                        get a value of the event (evt)
#define evt_take(evt)                 ( EVT_GET(evt) )
//      evt_give                        set a new value (val) of the event (evt)
#define evt_give(evt, val)         do { EVT_PUT(evt, val);                         } while(0)

/* Mutex ==================================================================== */

typedef struct __mtx { tsk_t *owner; } mtx_t;

/* Private ------------------------------------------------------------------ */
#define MTX_CLR(mtx)               do { (mtx)->owner = 0;       } while(0)
#define MTX_SET(mtx)               do { (mtx)->owner = Current; } while(0)
#define MTX_GET(mtx)                  ( (mtx)->owner )

/* Public ------------------------------------------------------------------- */
#define  OS_MTX(mtx)                    mtx_t mtx[1] = { { 0 } }
/* -------------------------------------------------------------------------- */
//      mtx_wait                        wait for the mutex (mtx)
#define mtx_wait(mtx)              do { tsk_waitWhile(MTX_GET(mtx)); MTX_SET(mtx); } while(0)
//      mtx_give                        release the owned mutex (mtx)
#define mtx_give(mtx)              do { if (MTX_GET(mtx) == Current) MTX_CLR(mtx); } while(0)

/* -------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif//__DUDOS
