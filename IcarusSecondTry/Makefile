# Reproducible command-line build for the static-IP LwIP ping diagnostic.
#
# CubeMX owns peripheral and middleware configuration in IcarusSecondTry.ioc.
# This Makefile deliberately writes generated artifacts outside the repository,
# so it can be used for build and linker-map acceptance without committing
# binaries or IDE state.

ARM_GCC ?= arm-none-eabi-gcc
ARM_OBJCOPY ?= arm-none-eabi-objcopy
HOST_CC ?= gcc
BUILD_DIR ?= /tmp/icarus-second-try-build
EXTRA_CPPFLAGS ?=
EXTRA_LDFLAGS ?=
NEWLIB_SPECS_DIR ?=
NEWLIB_LIBRARY_DIR ?=

TARGET := $(BUILD_DIR)/IcarusSecondTry
ELF := $(TARGET).elf
BIN := $(TARGET).bin
HEX := $(TARGET).hex
MAP := $(TARGET).map

INCLUDES := \
  -ICore/Inc \
  -Iservices \
  -Idrivers \
  -Iapp \
  -IDrivers/STM32H7xx_HAL_Driver/Inc \
  -IDrivers/STM32H7xx_HAL_Driver/Inc/Legacy \
  -IDrivers/CMSIS/Device/ST/STM32H7xx/Include \
  -IDrivers/CMSIS/Include \
  -IMiddlewares/Third_Party/FreeRTOS/Source/include \
  -IMiddlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 \
  -IMiddlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F \
  -ILWIP/App \
  -ILWIP/Target \
  -IMiddlewares/Third_Party/LwIP/src/include \
  -IMiddlewares/Third_Party/LwIP/system \
  -IMiddlewares/Third_Party/LwIP/system/arch

CPPFLAGS := $(INCLUDES) -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H743xx
CPUFLAGS := -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
CFLAGS := $(CPUFLAGS) -std=gnu11 -g3 -O0 -Wall -Wextra \
  -ffunction-sections -fdata-sections
SPECS_FLAGS := $(if $(NEWLIB_SPECS_DIR),\
  --specs=$(NEWLIB_SPECS_DIR)/nano.specs --specs=$(NEWLIB_SPECS_DIR)/nosys.specs,\
  --specs=nano.specs --specs=nosys.specs)
LDFLAGS := $(CPUFLAGS) -nostartfiles -TSTM32H743IITX_FLASH.ld $(SPECS_FLAGS) \
  $(if $(NEWLIB_LIBRARY_DIR),-L$(NEWLIB_LIBRARY_DIR)) $(EXTRA_LDFLAGS) \
  -Wl,--gc-sections,-Map=$(MAP) -Wl,--start-group \
  -lc -lm -Wl,--end-group

C_SOURCES := $(shell find Core/Src Drivers/STM32H7xx_HAL_Driver/Src \
  services app drivers board \
  Middlewares/Third_Party/FreeRTOS/Source \
  Middlewares/Third_Party/LwIP/src Middlewares/Third_Party/LwIP/system/OS \
  LWIP -type f -name '*.c' | sort)
S_SOURCES := Core/Startup/startup_stm32h743iitx.s
OBJECTS := $(addprefix $(BUILD_DIR)/,$(C_SOURCES:.c=.o) $(S_SOURCES:.s=.o))

.PHONY: all clean map-check host-test

all: $(ELF) $(BIN) $(HEX)

$(ELF): $(OBJECTS) STM32H743IITX_FLASH.ld

	@mkdir -p $(dir $@)
	$(ARM_GCC) $(OBJECTS) $(LDFLAGS) -o $@
	$(ARM_GCC) -Wl,--print-memory-usage -nostartfiles $(OBJECTS) $(LDFLAGS) -o /dev/null

$(BIN): $(ELF)
	$(ARM_OBJCOPY) -O binary $< $@

$(HEX): $(ELF)
	$(ARM_OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(ARM_GCC) $(CPPFLAGS) $(EXTRA_CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(ARM_GCC) $(CPPFLAGS) $(EXTRA_CPPFLAGS) $(CPUFLAGS) -x assembler-with-cpp -c $< -o $@

map-check: $(ELF)
	@grep -E 'DMARxDscrTab|DMATxDscrTab|memp_memory_RX_POOL_base|^\.lwip_heap' $(MAP)

host-test:
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Iservices services/protocol.c services/command.c services/safety.c tests/protocol_safety_test.c -o /tmp/icarus-protocol-safety-test
	/tmp/icarus-protocol-safety-test
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Iservices -Iapp services/protocol.c services/command.c services/safety.c app/control_runtime.c tests/control_runtime_test.c -o /tmp/icarus-control-runtime-test
	/tmp/icarus-control-runtime-test
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Iservices services/health.c tests/health_test.c -o /tmp/icarus-health-test
	/tmp/icarus-health-test
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Idrivers drivers/ad7689.c tests/ad7689_protocol_test.c -o /tmp/icarus-ad7689-test
	/tmp/icarus-ad7689-test
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Idrivers drivers/flowmeter.c tests/flowmeter_test.c -o /tmp/icarus-flowmeter-test
	/tmp/icarus-flowmeter-test
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Idrivers drivers/mcp23s17.c tests/mcp23s17_protocol_test.c -o /tmp/icarus-mcp23s17-test
	/tmp/icarus-mcp23s17-test
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Idrivers drivers/vesc_uart.c tests/vesc_uart_test.c -o /tmp/icarus-vesc-uart-test
	/tmp/icarus-vesc-uart-test
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Iservices -Idrivers -Iapp services/safety.c services/command.c drivers/mcp23s17.c drivers/vesc_uart.c app/actuator_service.c tests/actuator_service_test.c -o /tmp/icarus-actuator-service-test
	/tmp/icarus-actuator-service-test
	python3 tools/test_icarus_protocol.py

clean:
	rm -rf $(BUILD_DIR)

-include $(OBJECTS:.o=.d)
