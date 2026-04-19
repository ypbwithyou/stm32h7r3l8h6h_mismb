# CMake configuration for STM32H7R3xx project
# Target: atk_h7r3
# Device: STM32H7R3L8HxH
# Platform: ARM Cortex-M7 with FPU

cmake_minimum_required(VERSION 3.20)

# Set toolchain file before project() command
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/gcc-arm-none-eabi.cmake CACHE STRING "Toolchain file")

# Project name
project(atk_h7r3 C CXX ASM)

# C/C++ Standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# Project Options
# =============================================================================
option(USE_HAL_DRIVER "Use HAL Driver" ON)
option(USE_USB_HS "Use USB High Speed" ON)
option(UPGRADE_ON "Enable firmware upgrade mode" ON)

# =============================================================================
# MCU Configuration
# =============================================================================
set(MCU_FLAGS "-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard")
set(OPT_FLAGS "-O2 -g")
set(WARN_FLAGS "-Wall -Wextra -Wpedantic")
set(DEBUG_FLAGS "-gdwarf-4")

# C Flags
set(CMAKE_C_FLAGS "${MCU_FLAGS} ${OPT_FLAGS} ${WARN_FLAGS} ${DEBUG_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdata-sections -ffunction-sections")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-usage")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter -Wno-missing-field-initializers")

# C++ Flags
set(CMAKE_CXX_FLAGS "${MCU_FLAGS} ${OPT_FLAGS} ${WARN_FLAGS} ${DEBUG_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdata-sections -ffunction-sections -fno-rtti -fno-exceptions")

# Assembler Flags
set(CMAKE_ASM_FLAGS "${MCU_FLAGS} -x assembler-with-cpp -Wall -fdata-sections -ffunction-sections")

# Linker Flags
set(CMAKE_EXE_LINKER_FLAGS "${MCU_FLAGS} -specs=nano.specs -specs=nosys.specs")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T${CMAKE_SOURCE_DIR}/cmake/STM32H7R3xx_FLASH.ld")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -Wl,--print-memory-usage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -u _printf_float -u _scanf_float")

# =============================================================================
# Defines
# =============================================================================
add_compile_definitions(
    USE_HAL_DRIVER
    STM32H7R3xx
    USE_USB_HS
    $<$<BOOL:${UPGRADE_ON}>:UPGRADE_ON>
)

# =============================================================================
# Include Directories
# =============================================================================
include_directories(
${CMAKE_SOURCE_DIR}
${CMAKE_SOURCE_DIR}/Drivers
${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32H7RSxx/Include
${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Inc
${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Inc/Legacy
${CMAKE_SOURCE_DIR}/Drivers/BSP
${CMAKE_SOURCE_DIR}/Drivers/BSP/LED
${CMAKE_SOURCE_DIR}/Drivers/BSP/HYPERRAM
${CMAKE_SOURCE_DIR}/Drivers/BSP/NORFLASH
${CMAKE_SOURCE_DIR}/Drivers/BSP/IIC
${CMAKE_SOURCE_DIR}/Drivers/BSP/PCA9554A
${CMAKE_SOURCE_DIR}/Drivers/BSP/RS485
${CMAKE_SOURCE_DIR}/Drivers/BSP/SPI
${CMAKE_SOURCE_DIR}/Drivers/BSP/TIMER
${CMAKE_SOURCE_DIR}/Drivers/BSP/ADS8319
${CMAKE_SOURCE_DIR}/Drivers/BSP/MMC
${CMAKE_SOURCE_DIR}/Drivers/BSP/SDNAND
${CMAKE_SOURCE_DIR}/Drivers/BSP/RTC
${CMAKE_SOURCE_DIR}/Drivers/BSP/SDMMC
${CMAKE_SOURCE_DIR}/Drivers/SYSTEM
${CMAKE_SOURCE_DIR}/Drivers/SYSTEM/sys
${CMAKE_SOURCE_DIR}/Drivers/SYSTEM/delay
${CMAKE_SOURCE_DIR}/Drivers/SYSTEM/usart
${CMAKE_SOURCE_DIR}/Drivers/LIBS
${CMAKE_SOURCE_DIR}/Drivers/LIBS/lib_base_calculation
${CMAKE_SOURCE_DIR}/Drivers/LIBS/lib_circular_buffer
${CMAKE_SOURCE_DIR}/Drivers/LIBS/lib_dwt
${CMAKE_SOURCE_DIR}/Drivers/LIBS/lib_file_utils
${CMAKE_SOURCE_DIR}/Drivers/LIBS/lib_usb_protocol
${CMAKE_SOURCE_DIR}/Drivers/LIBS/lib_ini_fatfs
${CMAKE_SOURCE_DIR}/Middlewares
${CMAKE_SOURCE_DIR}/Middlewares/MALLOC
${CMAKE_SOURCE_DIR}/Middlewares/FATFS
${CMAKE_SOURCE_DIR}/Middlewares/FATFS/source
${CMAKE_SOURCE_DIR}/Middlewares/FATFS/exfuns
${CMAKE_SOURCE_DIR}/Middlewares/USB
${CMAKE_SOURCE_DIR}/Middlewares/USB/USB_APP
${CMAKE_SOURCE_DIR}/Middlewares/USB/STM32_USB_Device_Library/Core/Inc
${CMAKE_SOURCE_DIR}/Middlewares/USB/STM32_USB_Device_Library/Class/CDC/Inc
${CMAKE_SOURCE_DIR}/User
)

# =============================================================================
# Source Files - Startup
# =============================================================================
set(STARTUP_SOURCES
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32H7RSxx/Source/Templates/gcc/startup_stm32h7r3xx.s
)

# Use GCC startup file if exists, otherwise create from ARM version
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32H7RSxx/Source/Templates/gcc/startup_stm32h7r3xx.s)
    # Check if we need to create GCC startup
    file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32H7RSxx/Source/Templates/gcc)
    # Will use inline assembly or generic startup
    set(STARTUP_SOURCES "")
endif()

# =============================================================================
# Source Files - CMSIS
# =============================================================================
set(CMSIS_SOURCES
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32H7RSxx/Source/Templates/system_stm32h7rsxx.c
)

# =============================================================================
# Source Files - HAL Driver
# =============================================================================
set(HAL_SOURCES
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_cortex.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_pwr.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_pwr_ex.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_rcc.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_rcc_ex.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_gpio.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_uart.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_uart_ex.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_dma.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_dma_ex.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_xspi.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_sram.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_ll_fmc.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_spi.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_spi_ex.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_ll_spi.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_sd.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_sd_ex.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_ll_sdmmc.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_tim.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_tim_ex.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_ll_tim.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_i2c.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_i2c_ex.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_mmc.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_mmc_ex.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_pcd.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_pcd_ex.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_ll_i2c.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_ll_usb.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_rtc.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_hal_rtc_ex.c
    ${CMAKE_SOURCE_DIR}/Drivers/STM32H7RSxx_HAL_Driver/Src/stm32h7rsxx_ll_rtc.c
)

# =============================================================================
# Source Files - BSP Drivers
# =============================================================================
set(BSP_SOURCES
    ${CMAKE_SOURCE_DIR}/Drivers/BSP/LED/led.c
    ${CMAKE_SOURCE_DIR}/Drivers/BSP/HYPERRAM/hyperram.c
    ${CMAKE_SOURCE_DIR}/Drivers/BSP/NORFLASH/norflash.c
    ${CMAKE_SOURCE_DIR}/Drivers/BSP/NORFLASH/norflash_ex.c
    ${CMAKE_SOURCE_DIR}/Drivers/BSP/NORFLASH/norflash_mx25um25645g.c
    ${CMAKE_SOURCE_DIR}/Drivers/BSP/NORFLASH/norflash_w25q128_dual.c
    ${CMAKE_SOURCE_DIR}/Drivers/BSP/IIC/myiic.c
    ${CMAKE_SOURCE_DIR}/Drivers/BSP/PCA9554A/pca9554a.c
    ${CMAKE_SOURCE_DIR}/Drivers/BSP/RS485/rs485.c
    ${CMAKE_SOURCE_DIR}/Drivers/BSP/SPI/spi.c
    ${CMAKE_SOURCE_DIR}/Drivers/BSP/TIMER/gtim.c
    ${CMAKE_SOURCE_DIR}/Drivers/BSP/ADS8319/ads8319.c
    ${CMAKE_SOURCE_DIR}/Drivers/BSP/MMC/mmc_sdcard.c
    ${CMAKE_SOURCE_DIR}/Drivers/BSP/SDNAND/spi_sdnand.c
    ${CMAKE_SOURCE_DIR}/Drivers/BSP/RTC/rtc.c
)

# =============================================================================
# Source Files - System Drivers
# =============================================================================
set(SYSTEM_SOURCES
    ${CMAKE_SOURCE_DIR}/Drivers/SYSTEM/sys/sys.c
    ${CMAKE_SOURCE_DIR}/Drivers/SYSTEM/delay/delay.c
    ${CMAKE_SOURCE_DIR}/Drivers/SYSTEM/usart/usart.c
)

# =============================================================================
# Source Files - Library
# =============================================================================
set(LIBS_SOURCES
    ${CMAKE_SOURCE_DIR}/Drivers/LIBS/lib_base_calculation/base_calculation.c
    ${CMAKE_SOURCE_DIR}/Drivers/LIBS/lib_base_calculation/calculate_check.c
    ${CMAKE_SOURCE_DIR}/Drivers/LIBS/lib_circular_buffer/CircularBuffer.c
    ${CMAKE_SOURCE_DIR}/Drivers/LIBS/lib_dwt/lib_dwt_timestamp.c
    ${CMAKE_SOURCE_DIR}/Drivers/LIBS/lib_file_utils/file_utils.c
    ${CMAKE_SOURCE_DIR}/Drivers/LIBS/lib_file_utils/file_utils_disk.c
    ${CMAKE_SOURCE_DIR}/Drivers/LIBS/lib_ini_fatfs/ini_fatfs.c
    ${CMAKE_SOURCE_DIR}/Drivers/LIBS/lib_usb_protocol/slidingWindowReceiver_c.c
    ${CMAKE_SOURCE_DIR}/Drivers/LIBS/lib_usb_protocol/usb_protocol.c
)

# =============================================================================
# Source Files - Middlewares
# =============================================================================
set(MIDDLEWARES_SOURCES
    # MALLOC
    ${CMAKE_SOURCE_DIR}/Middlewares/MALLOC/malloc.c
    # FATFS
    ${CMAKE_SOURCE_DIR}/Middlewares/FATFS/source/diskio.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FATFS/source/ff.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FATFS/source/ffsystem.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FATFS/source/ffunicode.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FATFS/exfuns/exfuns.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FATFS/exfuns/fattester.c
    # USB Core
    ${CMAKE_SOURCE_DIR}/Middlewares/USB/STM32_USB_Device_Library/Core/Src/usbd_core.c
    ${CMAKE_SOURCE_DIR}/Middlewares/USB/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c
    ${CMAKE_SOURCE_DIR}/Middlewares/USB/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c
    # USB CDC Class
    ${CMAKE_SOURCE_DIR}/Middlewares/USB/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c
    # USB Application
    ${CMAKE_SOURCE_DIR}/Middlewares/USB/USB_APP/usbd_cdc_if.c
    ${CMAKE_SOURCE_DIR}/Middlewares/USB/USB_APP/usbd_conf.c
    ${CMAKE_SOURCE_DIR}/Middlewares/USB/USB_APP/usbd_desc.c
)

# =============================================================================
# Source Files - User Application
# =============================================================================
set(USER_SOURCES
    ${CMAKE_SOURCE_DIR}/User/main.c
    ${CMAKE_SOURCE_DIR}/User/stm32h7rsxx_it.c
    ${CMAKE_SOURCE_DIR}/User/collector_processor.c
    ${CMAKE_SOURCE_DIR}/User/ida_config.c
    ${CMAKE_SOURCE_DIR}/User/offline_processor.c
    ${CMAKE_SOURCE_DIR}/User/rs485_processor.c
    ${CMAKE_SOURCE_DIR}/User/usb_processor.c
    ${CMAKE_SOURCE_DIR}/User/dataType.c
    ${CMAKE_SOURCE_DIR}/User/bridge_config.c
)

# =============================================================================
# Combine all sources
# =============================================================================
set(SOURCES
    ${STARTUP_SOURCES}
    ${CMSIS_SOURCES}
    ${HAL_SOURCES}
    ${BSP_SOURCES}
    ${SYSTEM_SOURCES}
    ${LIBS_SOURCES}
    ${MIDDLEWARES_SOURCES}
    ${USER_SOURCES}
)

# =============================================================================
# Create Executable
# =============================================================================
add_executable(${PROJECT_NAME}.elf ${SOURCES})

# Set target properties
set_target_properties(${PROJECT_NAME}.elf PROPERTIES
    SUFFIX ""
    LINKER_LANGUAGE C
)

# =============================================================================
# Post-build commands
# =============================================================================
# Create binary and hex files
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
    COMMENT "Generating binary and hex files..."
)

# =============================================================================
# Print configuration
# =============================================================================
message(STATUS "")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Target: STM32H7R3L8HxH")
message(STATUS "CPU: Cortex-M7")
message(STATUS "FPU: FPv5-D16 (Hard)")
message(STATUS "")
message(STATUS "Source files: ${SOURCES}")
message(STATUS "")
