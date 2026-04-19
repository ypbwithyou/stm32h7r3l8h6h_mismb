/**************************************************************************//**
 * @file cmsis_gcc.h
 * @brief CMSIS compiler GCC header file
 * @version V5.4.3
 * @date 27. May 2021
 ******************************************************************************/
/*
 * Copyright (c) 2009-2021 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __CMSIS_GCC_H
#define __CMSIS_GCC_H

/* CMSIS compiler specific defines */
#ifndef __ASM
#define __ASM __asm
#endif
#ifndef __INLINE
#define __INLINE inline
#endif
#ifndef __STATIC_INLINE
#define __STATIC_INLINE static inline
#endif
#ifndef __STATIC_FORCEINLINE
#define __STATIC_FORCEINLINE __attribute__((always_inline)) static inline
#endif
#ifndef __NO_RETURN
#define __NO_RETURN __attribute__((__noreturn__))
#endif
#ifndef __USED
#define __USED __attribute__((used))
#endif
#ifndef __WEAK
#define __WEAK __attribute__((weak))
#endif
#ifndef __PACKED
#define __PACKED __attribute__((packed, aligned(1)))
#endif
#ifndef __PACKED_STRUCT
#define __PACKED_STRUCT struct __attribute__((packed, aligned(1)))
#endif
#ifndef __PACKED_UNION
#define __PACKED_UNION union __attribute__((packed, aligned(1)))
#endif
#ifndef __UNALIGNED_UINT32 /* deprecated */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpacked"
#pragma GCC diagnostic ignored "-Wattributes"
struct __attribute__((packed)) T_UINT32 { uint32_t v; };
#pragma GCC diagnostic pop
#define __UNALIGNED_UINT32(x) (((struct T_UINT32 *)(x))->v)
#endif
#ifndef __UNALIGNED_UINT16_WRITE
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpacked"
#pragma GCC diagnostic ignored "-Wattributes"
__PACKED_STRUCT T_UINT16_WRITE { uint16_t v; };
#pragma GCC diagnostic pop
#define __UNALIGNED_UINT16_WRITE(addr, val) (void)((((struct T_UINT16_WRITE *)(void *)(addr))->v) = (val))
#endif
#ifndef __UNALIGNED_UINT16_READ
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpacked"
#pragma GCC diagnostic ignored "-Wattributes"
__PACKED_STRUCT T_UINT16_READ { uint16_t v; };
#pragma GCC diagnostic pop
#define __UNALIGNED_UINT16_READ(addr) (((const struct T_UINT16_READ *)(const void *)(addr))->v)
#endif
#ifndef __UNALIGNED_UINT32_WRITE
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpacked"
#pragma GCC diagnostic ignored "-Wattributes"
__PACKED_STRUCT T_UINT32_WRITE { uint32_t v; };
#pragma GCC diagnostic pop
#define __UNALIGNED_UINT32_WRITE(addr, val) (void)((((struct T_UINT32_WRITE *)(void *)(addr))->v) = (val))
#endif
#ifndef __UNALIGNED_UINT32_READ
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpacked"
#pragma GCC diagnostic ignored "-Wattributes"
__PACKED_STRUCT T_UINT32_READ { uint32_t v; };
#pragma GCC diagnostic pop
#define __UNALIGNED_UINT32_READ(addr) (((const struct T_UINT32_READ *)(const void *)(addr))->v)
#endif
#ifndef __ALIGNED
#define __ALIGNED(x) __attribute__((aligned(x)))
#endif
#ifndef __RESTRICT
#define __RESTRICT __restrict
#endif
#ifndef __COMPILER_BARRIER
#define __COMPILER_BARRIER() __ASM volatile("":::"memory")
#endif

/* ######################### Startup and Lowlevel Init ######################## */

#ifndef __PROGRAM_START
#define __PROGRAM_START __main
#endif

#ifndef __INITIAL_SP
#define __INITIAL_SP Image$$ARM_LIB_STACK$$ZI$$Limit
#endif

#ifndef __STACK_LIMIT
#define __STACK_LIMIT Image$$ARM_LIB_STACK$$ZI$$Base
#endif

#ifndef __VECTOR_TABLE
#define __VECTOR_TABLE __Vectors
#endif

#ifndef __VECTOR_TABLE_ATTRIBUTE
#define __VECTOR_TABLE_ATTRIBUTE __attribute__((used, section("RESET")))
#endif

/* ########################## Core Instruction Access ######################### */

/* Define macros for porting to both thumb1 and thumb2.
 * For thumb1, use low register (r0-r7), specified by constraint "l"
 * Otherwise, use general registers, specified by constraint "r" */
#if defined (__thumb__) && !defined (__thumb2__)
#define __CMSIS_GCC_OUT_REG(r) "=l" (r)
#define __CMSIS_GCC_RW_REG(r) "+l" (r)
#define __CMSIS_GCC_USE_REG(r) "l" (r)
#else
#define __CMSIS_GCC_OUT_REG(r) "=r" (r)
#define __CMSIS_GCC_RW_REG(r) "+r" (r)
#define __CMSIS_GCC_USE_REG(r) "r" (r)
#endif

__STATIC_FORCEINLINE void __enable_irq(void)
{
    __ASM volatile ("cpsie i" : : : "memory");
}

__STATIC_FORCEINLINE void __disable_irq(void)
{
    __ASM volatile ("cpsid i" : : : "memory");
}

__STATIC_FORCEINLINE uint32_t __get_CONTROL(void)
{
    uint32_t result;
    __ASM volatile ("MRS %0, control" : "=r" (result) );
    return(result);
}

__STATIC_FORCEINLINE void __set_CONTROL(uint32_t control)
{
    __ASM volatile ("MSR control, %0" : : "r" (control) : "memory");
    __COMPILER_BARRIER();
}

__STATIC_FORCEINLINE uint32_t __get_IPSR(void)
{
    uint32_t result;
    __ASM volatile ("MRS %0, ipsr" : "=r" (result) );
    return(result);
}

__STATIC_FORCEINLINE uint32_t __get_APSR(void)
{
    uint32_t result;
    __ASM volatile ("MRS %0, apsr" : "=r" (result) );
    return(result);
}

__STATIC_FORCEINLINE uint32_t __get_xPSR(void)
{
    uint32_t result;
    __ASM volatile ("MRS %0, xpsr" : "=r" (result) );
    return(result);
}

__STATIC_FORCEINLINE uint32_t __get_PSP(void)
{
    uint32_t result;
    __ASM volatile ("MRS %0, psp" : "=r" (result) );
    return(result);
}

__STATIC_FORCEINLINE void __set_PSP(uint32_t topOfProcStack)
{
    __ASM volatile ("MSR psp, %0" : : "r" (topOfProcStack) : );
}

__STATIC_FORCEINLINE uint32_t __get_MSP(void)
{
    uint32_t result;
    __ASM volatile ("MRS %0, msp" : "=r" (result) );
    return(result);
}

__STATIC_FORCEINLINE void __set_MSP(uint32_t topOfMainStack)
{
    __ASM volatile ("MSR msp, %0" : : "r" (topOfMainStack) : );
}

__STATIC_FORCEINLINE uint32_t __get_PRIMASK(void)
{
    uint32_t result;
    __ASM volatile ("MRS %0, primask" : "=r" (result) );
    return(result);
}

__STATIC_FORCEINLINE void __set_PRIMASK(uint32_t priMask)
{
    __ASM volatile ("MSR primask, %0" : : "r" (priMask) : "memory");
}

#if ((defined (__ARM_ARCH_7M__ ) && (__ARM_ARCH_7M__ == 1)) || \
     (defined (__ARM_ARCH_7EM__ ) && (__ARM_ARCH_7EM__ == 1)) || \
     (defined (__ARM_ARCH_8M_MAIN__ ) && (__ARM_ARCH_8M_MAIN__ == 1)) || \
     (defined (__ARM_ARCH_8_1M_MAIN__) && (__ARM_ARCH_8_1M_MAIN__ == 1)) )

__STATIC_FORCEINLINE void __enable_fault_irq(void)
{
    __ASM volatile ("cpsie f" : : : "memory");
}

__STATIC_FORCEINLINE void __disable_fault_irq(void)
{
    __ASM volatile ("cpsid f" : : : "memory");
}

__STATIC_FORCEINLINE uint32_t __get_BASEPRI(void)
{
    uint32_t result;
    __ASM volatile ("MRS %0, basepri" : "=r" (result) );
    return(result);
}

__STATIC_FORCEINLINE void __set_BASEPRI(uint32_t basePri)
{
    __ASM volatile ("MSR basepri, %0" : : "r" (basePri) : "memory");
}

__STATIC_FORCEINLINE void __set_BASEPRI_MAX(uint32_t basePri)
{
    __ASM volatile ("MSR basepri_max, %0" : : "r" (basePri) : "memory");
}

__STATIC_FORCEINLINE uint32_t __get_FAULTMASK(void)
{
    uint32_t result;
    __ASM volatile ("MRS %0, faultmask" : "=r" (result) );
    return(result);
}

__STATIC_FORCEINLINE void __set_FAULTMASK(uint32_t faultMask)
{
    __ASM volatile ("MSR faultmask, %0" : : "r" (faultMask) : "memory");
}

#endif /* ((defined (__ARM_ARCH_7M__ ) && (__ARM_ARCH_7M__ == 1)) || \
          (defined (__ARM_ARCH_7EM__ ) && (__ARM_ARCH_7EM__ == 1)) || \
          (defined (__ARM_ARCH_8M_MAIN__ ) && (__ARM_ARCH_8M_MAIN__ == 1)) || \
          (defined (__ARM_ARCH_8_1M_MAIN__) && (__ARM_ARCH_8_1M_MAIN__ == 1)) ) */

/* ###########################  Core Function Access  ########################### */

__STATIC_FORCEINLINE uint8_t __CLZ(uint32_t value)
{
    if (value == 0U)
    {
        return 32U;
    }
    return __builtin_clz(value);
}

__STATIC_FORCEINLINE uint32_t __RBIT(uint32_t value)
{
    uint32_t result;
    __ASM volatile ("rbit %0, %1" : "=r" (result) : "r" (value) );
    return(result);
}

__STATIC_FORCEINLINE uint32_t __REV(uint32_t value)
{
    return __builtin_bswap32(value);
}

__STATIC_FORCEINLINE uint32_t __REV16(uint32_t value)
{
    uint32_t result;
    __ASM volatile ("rev16 %0, %1" : "=r" (result) : "r" (value) );
    return(result);
}

__STATIC_FORCEINLINE int32_t __REVSH(int32_t value)
{
    return (int16_t)__builtin_bswap16(value);
}

__STATIC_FORCEINLINE uint32_t __ROR(uint32_t op1, uint32_t op2)
{
  op2 %= 32U;
  if (op2 == 0U)
  {
    return op1;
  }
  return (op1 >> op2) | (op1 << (32U - op2));
}

/**
  \brief   Load-Exclusive (8 bit)
  \details Executes a LDREXB instruction.
  \param [in]    ptr  Pointer to data
  \return             value of type uint8_t at (*ptr)
 */
__STATIC_FORCEINLINE uint8_t __LDREXB(volatile uint8_t *addr)
{
    uint32_t result;

   __ASM volatile ("ldrexb %0, %1" : "=r" (result) : "Q" (*addr) );
   return ((uint8_t) result);
}


/**
  \brief   Load-Exclusive (16 bit)
  \details Executes a LDREXH instruction.
  \param [in]    ptr  Pointer to data
  \return        value of type uint16_t at (*ptr)
 */
__STATIC_FORCEINLINE uint16_t __LDREXH(volatile uint16_t *addr)
{
    uint32_t result;

   __ASM volatile ("ldrexh %0, %1" : "=r" (result) : "Q" (*addr) );
   return ((uint16_t) result);
}


/**
  \brief   Load-Exclusive (32 bit)
  \details Executes a LDREX instruction.
  \param [in]    ptr  Pointer to data
  \return        value of type uint32_t at (*ptr)
 */
__STATIC_FORCEINLINE uint32_t __LDREXW(volatile uint32_t *addr)
{
    uint32_t result;

   __ASM volatile ("ldrex %0, %1" : "=r" (result) : "Q" (*addr) );
   return(result);
}


/**
  \brief   Store-Exclusive (8 bit)
  \details Executes a STREXB instruction.
  \param [in]  value  Value to store
  \param [in]    ptr  Pointer to location
  \return          0  Function succeeded
  \return          1  Function failed
 */
__STATIC_FORCEINLINE uint32_t __STREXB(uint8_t value, volatile uint8_t *addr)
{
   uint32_t result;

   __ASM volatile ("strexb %0, %2, %1" : "=&r" (result), "=Q" (*addr) : "r" ((uint32_t)value) );
   return(result);
}


/**
  \brief   Store-Exclusive (16 bit)
  \details Executes a STREXH instruction.
  \param [in]  value  Value to store
  \param [in]    ptr  Pointer to location
  \return          0  Function succeeded
  \return          1  Function failed
 */
__STATIC_FORCEINLINE uint32_t __STREXH(uint16_t value, volatile uint16_t *addr)
{
   uint32_t result;

   __ASM volatile ("strexh %0, %2, %1" : "=&r" (result), "=Q" (*addr) : "r" ((uint32_t)value) );
   return(result);
}


/**
  \brief   Store-Exclusive (32 bit)
  \details Executes a STREX instruction.
  \param [in]  value  Value to store
  \param [in]    ptr  Pointer to location
  \return          0  Function succeeded
  \return          1  Function failed
 */
__STATIC_FORCEINLINE uint32_t __STREXW(uint32_t value, volatile uint32_t *addr)
{
   uint32_t result;

   __ASM volatile ("strex %0, %2, %1" : "=&r" (result), "=Q" (*addr) : "r" (value) );
   return(result);
}


/**
  \brief   Remove the exclusive lock
  \details Executes a CLREX instruction.
 */
__STATIC_FORCEINLINE void __CLREX(void)
{
  __ASM volatile ("clrex" ::: "memory");
}

__STATIC_FORCEINLINE void __NOP(void)
{
  __ASM volatile ("nop");
}

__STATIC_FORCEINLINE void __BKPT(uint8_t value)
{
  __ASM volatile ("bkpt %0" : : "i"(value));
}

__STATIC_FORCEINLINE void __WFI(void)
{
    __ASM volatile ("wfi");
}

__STATIC_FORCEINLINE void __WFE(void)
{
    __ASM volatile ("wfe");
}

__STATIC_FORCEINLINE void __SEV(void)
{
    __ASM volatile ("sev");
}

__STATIC_FORCEINLINE void __ISB(void)
{
    __ASM volatile ("isb 0xF" ::: "memory");
}

__STATIC_FORCEINLINE void __DSB(void)
{
    __ASM volatile ("dsb 0xF" ::: "memory");
}

__STATIC_FORCEINLINE void __DMB(void)
{
    __ASM volatile ("dmb 0xF" ::: "memory");
}

__STATIC_FORCEINLINE uint32_t __get_FPSCR(void)
{
#if ((defined (__FPU_PRESENT) && (__FPU_PRESENT == 1U)) && \
     (defined (__FPU_USED ) && (__FPU_USED == 1U)) )
    uint32_t result;
    __ASM volatile ("MRS %0, fpscr" : "=r" (result) );
    return(result);
#else
    return(0U);
#endif
}

__STATIC_FORCEINLINE void __set_FPSCR(uint32_t fpscr)
{
#if ((defined (__FPU_PRESENT) && (__FPU_PRESENT == 1U)) && \
     (defined (__FPU_USED ) && (__FPU_USED == 1U)) )
    __ASM volatile ("MSR fpscr, %0" : : "r" (fpscr) : "vfpcc", "memory" );
#else
    (void)fpscr;
#endif
}

#endif /* __CMSIS_GCC_H */
