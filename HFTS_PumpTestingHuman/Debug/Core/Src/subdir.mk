################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/ad7124_ll.c \
../Core/Src/ad7689.c \
../Core/Src/daq_udp.c \
../Core/Src/main.c \
../Core/Src/mcp23s17.c \
../Core/Src/mcp23s17_stm32_hal.c \
../Core/Src/stm32h7xx_hal_msp.c \
../Core/Src/stm32h7xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32h7xx.c \
../Core/Src/tim1_freq_capture.c \
../Core/Src/vesc_uart.c 

OBJS += \
./Core/Src/ad7124_ll.o \
./Core/Src/ad7689.o \
./Core/Src/daq_udp.o \
./Core/Src/main.o \
./Core/Src/mcp23s17.o \
./Core/Src/mcp23s17_stm32_hal.o \
./Core/Src/stm32h7xx_hal_msp.o \
./Core/Src/stm32h7xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32h7xx.o \
./Core/Src/tim1_freq_capture.o \
./Core/Src/vesc_uart.o 

C_DEPS += \
./Core/Src/ad7124_ll.d \
./Core/Src/ad7689.d \
./Core/Src/daq_udp.d \
./Core/Src/main.d \
./Core/Src/mcp23s17.d \
./Core/Src/mcp23s17_stm32_hal.d \
./Core/Src/stm32h7xx_hal_msp.d \
./Core/Src/stm32h7xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32h7xx.d \
./Core/Src/tim1_freq_capture.d \
./Core/Src/vesc_uart.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -O0 -ffunction-sections -fdata-sections -Wall -mno-unaligned-access -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/ad7124_ll.cyclo ./Core/Src/ad7124_ll.d ./Core/Src/ad7124_ll.o ./Core/Src/ad7124_ll.su ./Core/Src/ad7689.cyclo ./Core/Src/ad7689.d ./Core/Src/ad7689.o ./Core/Src/ad7689.su ./Core/Src/daq_udp.cyclo ./Core/Src/daq_udp.d ./Core/Src/daq_udp.o ./Core/Src/daq_udp.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/mcp23s17.cyclo ./Core/Src/mcp23s17.d ./Core/Src/mcp23s17.o ./Core/Src/mcp23s17.su ./Core/Src/mcp23s17_stm32_hal.cyclo ./Core/Src/mcp23s17_stm32_hal.d ./Core/Src/mcp23s17_stm32_hal.o ./Core/Src/mcp23s17_stm32_hal.su ./Core/Src/stm32h7xx_hal_msp.cyclo ./Core/Src/stm32h7xx_hal_msp.d ./Core/Src/stm32h7xx_hal_msp.o ./Core/Src/stm32h7xx_hal_msp.su ./Core/Src/stm32h7xx_it.cyclo ./Core/Src/stm32h7xx_it.d ./Core/Src/stm32h7xx_it.o ./Core/Src/stm32h7xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32h7xx.cyclo ./Core/Src/system_stm32h7xx.d ./Core/Src/system_stm32h7xx.o ./Core/Src/system_stm32h7xx.su ./Core/Src/tim1_freq_capture.cyclo ./Core/Src/tim1_freq_capture.d ./Core/Src/tim1_freq_capture.o ./Core/Src/tim1_freq_capture.su ./Core/Src/vesc_uart.cyclo ./Core/Src/vesc_uart.d ./Core/Src/vesc_uart.o ./Core/Src/vesc_uart.su

.PHONY: clean-Core-2f-Src

