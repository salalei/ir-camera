include makefile.conf

TARGET := $(PROJECT)

ifndef CROSS_COMPILE
	CROSS_COMPILE = arm-none-eabi-
endif

ifeq ($(OS), Windows_NT)
	PLATFORM := win
else
	PLATFORM := linux
endif

BUILD_DIR := build
TARGET_DIR := target

CC := $(CROSS_COMPILE)gcc
AS := $(CROSS_COMPILE)gcc -x assembler-with-cpp
CP := $(CROSS_COMPILE)objcopy
SZ := $(CROSS_COMPILE)size

HEX := $(CP) -O ihex
BIN := $(CP) -O binary -S

CFLAGS += -mcpu=cortex-m3
CFLAGS += -mfloat-abi=soft
CFLAGS += -mthumb
CFLAGS += $(C_DEFS) $(INC)
CFLAGS += $(OPT)
CFLAGS += -Wall
CFLAGS += -fdata-sections -ffunction-sections -fno-strict-aliasing
CFLAGS += -MMD -MP -MF$(@:%.o=%.d)
CFLAGS += -D__STACK_SIZE=$(STACK_SIZE)
CFLAGS += -D__HEAP_SIZE=$(HEAP_SIZE)

ASMFLAGS += -mcpu=cortex-m3
ASMFLAGS += -mfloat-abi=soft
ASMFLAGS += -mthumb
ASMFLAGS += $(OPT)
ASMFLAGS += -D__STACK_SIZE=$(STACK_SIZE)
ASMFLAGS += -D__HEAP_SIZE=$(HEAP_SIZE)

LDFLAGS += -mcpu=cortex-m3
LDFLAGS += -mfloat-abi=soft
LDFLAGS += -mthumb
LDFLAGS += $(OPT)
LDFLAGS += -T$(LDSCRIPT)
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref,--gc-sections,-u,_printf_float,-lc,-lm,-lnosys,-lrdimon

OBJECTS += $(addprefix $(BUILD_DIR)/, $(notdir $(SRC:%.c=%.o)))
vpath %.c $(sort $(dir $(SRC)))
OBJECTS += $(addprefix $(BUILD_DIR)/, $(notdir $(STARTUP:%.S=%.o)))
vpath %.S $(sort $(dir $(STARTUP)))

all: path_check $(TARGET_DIR)/$(TARGET).hex $(TARGET_DIR)/$(TARGET).bin
	@echo Compile finish!

$(TARGET_DIR)/$(TARGET).hex: $(TARGET_DIR)/$(TARGET).elf
	@$(HEX) $< $@
	@echo HEX $@

$(TARGET_DIR)/$(TARGET).bin: $(TARGET_DIR)/$(TARGET).elf
	@$(BIN) $< $@
	@echo BIN $@

$(TARGET_DIR)/$(TARGET).elf: $(OBJECTS) $(LDSCRIPT)
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo LD $@...
	@$(SZ) $@

$(BUILD_DIR)/%.o: %.c Makefile makefile.conf
	@$(CC) -c $(CFLAGS) -Wa,-adlms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@
	@echo CC $<...

$(BUILD_DIR)/%.o: %.S Makefile makefile.conf
	@$(AS) -c $(ASMFLAGS) $< -o $@
	@echo AS $<...

path_check:
ifeq ($(PLATFORM), win)
	@if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	@if not exist $(TARGET_DIR) mkdir $(TARGET_DIR)
else
	@mkdir -p $(BUILD_DIR) $(TARGET_DIR)
endif

clean:
ifeq ($(PLATFORM), win)
	@del /q $(BUILD_DIR) $(TARGET_DIR)
else
	@rm -rf $(BUILD_DIR)/* $(TARGET_DIR)/*
endif

-include $(wildcard $(BUILD_DIR)/*.d)