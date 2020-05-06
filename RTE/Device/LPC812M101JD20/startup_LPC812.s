;/**************************************************************************//**
; * @file     startup_LPC812.s
; * @brief    CMSIS Core Device Startup File for
; *           NXP LPC812 Device Series
; * @version  1.2.0
; * @date     12. March 2018
; *
; * @note
; * Copyright (C) 2018 ARM Limited. All rights reserved.
; *
; * @par
; * ARM Limited (ARM) is supplying this software for use with Cortex-M
; * processor based microcontrollers.  This file can be freely distributed
; * within development tools that are supporting such ARM based processors.
; *
; * @par
; * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
; * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
; * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
; * ARM SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
; * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
; *
; ******************************************************************************/

; *------- <<< Use Configuration Wizard in Context Menu >>> ------------------

; <h> Stack Configuration
;   <o> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>

Stack_Size      EQU     0x00000100

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp


; <h> Heap Configuration
;   <o>  Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>

Heap_Size       EQU     0x00000000

                IF      Heap_Size != 0                ; Heap is provided
                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit
                ELSE
                IMPORT __use_no_heap                  ; generate warning if heap is required
                ENDIF


                PRESERVE8
                THUMB


; Vector Table Mapped to Address 0 at Reset

                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors

__Vectors       DCD     __initial_sp               ; Top of Stack
                DCD     Reset_Handler              ; Reset Handler
                DCD     NMI_Handler                ; NMI Handler
                DCD     HardFault_Handler          ; Hard Fault Handler
                DCD     0                          ; Reserved
                DCD     0                          ; Reserved
                DCD     0                          ; Reserved
                DCD     0                          ; Reserved
                DCD     0                          ; Reserved
                DCD     0                          ; Reserved
                DCD     0                          ; Reserved
                DCD     SVC_Handler                ; SVCall Handler
                DCD     0                          ; Reserved
                DCD     0                          ; Reserved
                DCD     PendSV_Handler             ; PendSV Handler
                DCD     SysTick_Handler            ; SysTick Handler

                ; External Interrupts
                DCD     SPI0_IRQHandler            ; SPI0 interrupt
                DCD     SPI1_IRQHandler            ; SPI1 interrupt
                DCD     0                          ; Reserved interrupt
                DCD     USART0_IRQHandler          ; USART0 interrupt
                DCD     USART1_IRQHandler          ; USART1 interrupt
                DCD     USART2_IRQHandler          ; USART2 interrupt
                DCD     0                          ; Reserved interrupt
                DCD     0                          ; Reserved interrupt
                DCD     I2C0_IRQHandler            ; I2C0 interrupt
                DCD     SCT0_IRQHandler            ; State configurable timer interrupt
                DCD     MRT0_IRQHandler            ; Multi-rate timer interrupt
                DCD     CMP_IRQHandler             ; Analog comparator interrupt or Capacitive Touch interrupt
                DCD     WDT_IRQHandler             ; Windowed watchdog timer interrupt
                DCD     BOD_IRQHandler             ; BOD interrupts
                DCD     0                          ; Reserved interrupt
                DCD     WKT_IRQHandler             ; Self-wake-up timer interrupt
                DCD     0                          ; Reserved interrupt
                DCD     0                          ; Reserved interrupt
                DCD     0                          ; Reserved interrupt
                DCD     0                          ; Reserved interrupt
                DCD     0                          ; Reserved interrupt
                DCD     0                          ; Reserved interrupt
                DCD     0                          ; Reserved interrupt
                DCD     0                          ; Reserved interrupt
                DCD     PIN_INT0_IRQHandler        ; Pin interrupt 0 or pattern match engine slice 0 interrupt
                DCD     PIN_INT1_IRQHandler        ; Pin interrupt 1 or pattern match engine slice 1 interrupt
                DCD     PIN_INT2_IRQHandler        ; Pin interrupt 2 or pattern match engine slice 2 interrupt
                DCD     PIN_INT3_IRQHandler        ; Pin interrupt 3 or pattern match engine slice 3 interrupt
                DCD     PIN_INT4_IRQHandler        ; Pin interrupt 4 or pattern match engine slice 4 interrupt
                DCD     PIN_INT5_IRQHandler        ; Pin interrupt 5 or pattern match engine slice 5 interrupt
                DCD     PIN_INT6_IRQHandler        ; Pin interrupt 6 or pattern match engine slice 6 interrupt
                DCD     PIN_INT7_IRQHandler        ; Pin interrupt 7 or pattern match engine slice 7 interrupt

; <h> Code Read Protection
;   <o> Code Read Protection  <0xFFFFFFFF=>CRP Disabled
;                             <0x12345678=>CRP Level 1
;                             <0x87654321=>CRP Level 2
;                             <0x43218765=>CRP Level 3 (ARE YOU SURE?)
;                             <0x4E697370=>NO ISP (ARE YOU SURE?)
; </h>
                IF      :LNOT::DEF:NO_CRP
                AREA    |.ARM.__at_0x02FC|, CODE, READONLY
                DCD     0xFFFFFFFF
                ENDIF

                AREA    |.text|, CODE, READONLY


; Reset Handler

Reset_Handler   PROC
                EXPORT  Reset_Handler              [WEAK]
                IMPORT  SystemInit
                IMPORT  __main
                LDR     R0, =SystemInit
                BLX     R0
                LDR     R0, =__main
                BX      R0
                ENDP


; Dummy Exception Handlers (infinite loops which can be modified)
NMI_Handler     PROC
                EXPORT  NMI_Handler                [WEAK]
                B       .
                ENDP
HardFault_Handler\
                PROC
                EXPORT  HardFault_Handler          [WEAK]
                B       .
                ENDP
SVC_Handler     PROC
                EXPORT  SVC_Handler                [WEAK]
                B       .
                ENDP
PendSV_Handler  PROC
                EXPORT  PendSV_Handler             [WEAK]
                B       .
                ENDP
SysTick_Handler PROC
                EXPORT  SysTick_Handler            [WEAK]
                B       .
                ENDP

Default_Handler PROC

                EXPORT  SPI0_IRQHandler            [WEAK]
                EXPORT  SPI1_IRQHandler            [WEAK]
                EXPORT  USART0_IRQHandler          [WEAK]
                EXPORT  USART1_IRQHandler          [WEAK]
                EXPORT  USART2_IRQHandler          [WEAK]
                EXPORT  I2C0_IRQHandler            [WEAK]
                EXPORT  SCT0_IRQHandler            [WEAK]
                EXPORT  MRT0_IRQHandler            [WEAK]
                EXPORT  CMP_IRQHandler             [WEAK]
                EXPORT  WDT_IRQHandler             [WEAK]
                EXPORT  BOD_IRQHandler             [WEAK]
                EXPORT  WKT_IRQHandler             [WEAK]
                EXPORT  PIN_INT0_IRQHandler        [WEAK]
                EXPORT  PIN_INT1_IRQHandler        [WEAK]
                EXPORT  PIN_INT2_IRQHandler        [WEAK]
                EXPORT  PIN_INT3_IRQHandler        [WEAK]
                EXPORT  PIN_INT4_IRQHandler        [WEAK]
                EXPORT  PIN_INT5_IRQHandler        [WEAK]
                EXPORT  PIN_INT6_IRQHandler        [WEAK]
                EXPORT  PIN_INT7_IRQHandler        [WEAK]

SPI0_IRQHandler
SPI1_IRQHandler
USART0_IRQHandler
USART1_IRQHandler
USART2_IRQHandler
I2C0_IRQHandler
SCT0_IRQHandler
MRT0_IRQHandler
CMP_IRQHandler
WDT_IRQHandler
BOD_IRQHandler
WKT_IRQHandler
PIN_INT0_IRQHandler
PIN_INT1_IRQHandler
PIN_INT2_IRQHandler
PIN_INT3_IRQHandler
PIN_INT4_IRQHandler
PIN_INT5_IRQHandler
PIN_INT6_IRQHandler
PIN_INT7_IRQHandler

                B       .

                ENDP


                ALIGN


; User setup Stack & Heap

                IF      :LNOT::DEF:__MICROLIB
                IMPORT  __use_two_region_memory
                ENDIF

                EXPORT  __initial_sp
                IF      Heap_Size != 0                ; Heap is provided
                EXPORT  __heap_base
                EXPORT  __heap_limit
                ENDIF

                END
