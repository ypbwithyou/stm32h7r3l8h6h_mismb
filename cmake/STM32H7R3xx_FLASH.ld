/*
 * GCC linker script for STM32H7R3xx with XSPI1 external Flash support
 * Memory Layout:
 * - Internal Flash: 0x08000000 - 0x08010000 (64KB)
 * - ITCM RAM: 0x00000000 - 0x00010000 (64KB)
 * - DTCM RAM: 0x20000000 - 0x20010000 (64KB)
 * - AXI RAM (SRAM1): 0x24000000 - 0x24072000 (456KB)
 * - AHBSRAM: 0x30000000 - 0x30008000 (32KB)
 * - Backup SRAM: 0x38800000 - 0x38801000 (4KB)
 * - External Flash via XSPI1: 0x90000000 - 0xA0000000 (256MB)
 * - External HyperRAM via XSPI2: 0x70000000 - 0x70800000 (8MB)
 */

/* Entry Point */
ENTRY(Reset_Handler)

/* Memory Definitions */
MEMORY
{
    /* Internal Flash - used for bootloader/vector table/core init code */
    IROM (rx) : ORIGIN = 0x08000000, LENGTH = 64K

    /* ITCM RAM */
    ITCM (rwx) : ORIGIN = 0x00000000, LENGTH = 64K

    /* DTCM RAM */
    DTCM (rwx) : ORIGIN = 0x20000000, LENGTH = 64K

    /* AXI RAM - Main RAM for data */
    SRAM1 (rwx) : ORIGIN = 0x24000000, LENGTH = 456K

    /* AHBSRAM */
    AHBSRAM (rwx) : ORIGIN = 0x30000000, LENGTH = 32K

    /* Backup SRAM */
    BKPSRAM (rwx) : ORIGIN = 0x38800000, LENGTH = 4K

    /* External Flash via XSPI1 - main code storage */
    XSPI1_FLASH (rx) : ORIGIN = 0x90000000, LENGTH = 256M

    /* External HyperRAM via XSPI2 - memory pool (8MB) */
    XSPI2_HYPERRAM (rwx) : ORIGIN = 0x70000000, LENGTH = 8M
}

/* Section Definitions */
SECTIONS
{
    /* Vector table at beginning of internal flash */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > IROM

    /* Core startup and initialization code in internal flash */
    .text.boot :
    {
        . = ALIGN(4);
        /* Startup code */
        *startup_stm32h7r3xx.o(.text .text*)
        *system_stm32h7rsxx.o(.text .text*)
        /* Core HAL files needed for boot */
        *stm32h7rsxx_hal.o(.text .text*)
        *stm32h7rsxx_hal_cortex.o(.text .text*)
        *stm32h7rsxx_hal_pwr*.o(.text .text*)
        *stm32h7rsxx_hal_rcc*.o(.text .text*)
        *stm32h7rsxx_hal_gpio.o(.text .text*)
        *stm32h7rsxx_hal_xspi.o(.text .text*)
        /* System files */
        *sys.o(.text .text*)
        *delay.o(.text .text*)
        *usart.o(.text .text*)
        /* Interrupt handler */
        *stm32h7rsxx_it.o(.text .text*)
        . = ALIGN(4);
    } > IROM

    /* Main application code in external XSPI1 Flash */
    .text.xspi1 :
    {
        . = ALIGN(4);
        _stext_xspi1 = .;
        /* All other .text sections */
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP (*(.init))
        KEEP (*(.fini))
        . = ALIGN(4);
        _etext_xspi1 = .;
    } > XSPI1_FLASH

    /* Constant data in external XSPI1 Flash */
    .rodata :
    {
        . = ALIGN(4);
        _srodata = .;
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _erodata = .;
    } > XSPI1_FLASH

    /* ARM C++ exception handling tables in external Flash */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > XSPI1_FLASH

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > XSPI1_FLASH

    /* Constructor/destructor tables in external Flash */
    .preinit_array :
    {
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    } > XSPI1_FLASH

    .init_array :
    {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
    } > XSPI1_FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
    } > XSPI1_FLASH

    /* Initialized data - loaded from external flash to RAM */
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        __data_start__ = _sdata;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
        __data_end__ = _edata;
    } > SRAM1 AT> XSPI1_FLASH

    /* 
     * Handle specially placed BSS sections from malloc.c
     * These use section names like .bss.ARM.__at_0x70000000
     * We need to place them at their specified addresses
     */

    /* mem2base at 0x70000000 - HyperRAM */
    .bss.ARM.__at_0x70000000 (NOLOAD) :
    {
        . = ALIGN(64);
        *(.bss.ARM.__at_0x70000000)
        . = ALIGN(64);
    } > XSPI2_HYPERRAM AT> XSPI2_HYPERRAM

    /* mem2mapbase at 0x70787878 - HyperRAM MAP */
    .bss.ARM.__at_0x70787878 (NOLOAD) :
    {
        . = ALIGN(4);
        *(.bss.ARM.__at_0x70787878)
    } > XSPI2_HYPERRAM AT> XSPI2_HYPERRAM

    /* mem3base at 0x30000000 - AHBSRAM */
    .bss.ARM.__at_0x30000000 (NOLOAD) :
    {
        . = ALIGN(64);
        *(.bss.ARM.__at_0x30000000)
        . = ALIGN(64);
    } > AHBSRAM AT> AHBSRAM

    /* mem3mapbase at 0x30007840 - AHBSRAM MAP */
    .bss.ARM.__at_0x30007840 (NOLOAD) :
    {
        . = ALIGN(4);
        *(.bss.ARM.__at_0x30007840)
    } > AHBSRAM AT> AHBSRAM

    /* mem4base at 0x20000000 - DTCM */
    .bss.ARM.__at_0x20000000 (NOLOAD) :
    {
        . = ALIGN(64);
        *(.bss.ARM.__at_0x20000000)
        . = ALIGN(64);
    } > DTCM AT> DTCM

    /* mem4mapbase at 0x2000F0C0 - DTCM MAP */
    .bss.ARM.__at_0x2000F0C0 (NOLOAD) :
    {
        . = ALIGN(4);
        *(.bss.ARM.__at_0x2000F0C0)
    } > DTCM AT> DTCM

    /* mem5base at 0x00000000 - ITCM */
    .bss.ARM.__at_0x00000000 (NOLOAD) :
    {
        . = ALIGN(64);
        *(.bss.ARM.__at_0x00000000)
        . = ALIGN(64);
    } > ITCM AT> ITCM

    /* mem5mapbase at 0x0000F0C0 - ITCM MAP */
    .bss.ARM.__at_0x0000F0C0 (NOLOAD) :
    {
        . = ALIGN(4);
        *(.bss.ARM.__at_0x0000F0C0)
    } > ITCM AT> ITCM

    /* Regular BSS in SRAM1 */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        /* Exclude sections already handled above */
        EXCLUDE_FILE(*malloc.o) *(.bss.mem*base)
        EXCLUDE_FILE(*malloc.o) *(.bss.mem*mapbase)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > SRAM1 AT> SRAM1

    /* Stack in DTCM (grows downward from end of DTCM) */
    __stack_bottom = ORIGIN(DTCM) + LENGTH(DTCM);
    __initial_sp = ORIGIN(DTCM) + LENGTH(DTCM);

    /* Heap in SRAM1 */
    _end = _ebss;
    __heap_start__ = _end;
    __heap_end__ = ORIGIN(SRAM1) + LENGTH(SRAM1);

    /* Provide end symbol for _sbrk */
    end = _end;

    /* Remove compiler libraries from link */
    /DISCARD/ :
    {
        libc.a (*)
        libm.a (*)
        libgcc.a (*)
    }

    /* DWARF debug sections */
    .debug_info 0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev 0 : { *(.debug_abbrev) }
    .debug_loc 0 : { *(.debug_loc) }
    .debug_aranges 0 : { *(.debug_aranges) }
    .debug_line 0 : { *(.debug_line .debug_line.* .debug_line_end) }
    .debug_str 0 : { *(.debug_str) }
    .debug_frame 0 : { *(.debug_frame) }
    .debug_macinfo 0 : { *(.debug_macinfo) }
    .debug_ranges 0 : { *(.debug_ranges) }
}
