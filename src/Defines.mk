SW_VERSION_MAJOR            := $(shell git describe --tags --abbrev=0 | cut -c 2- | cut -d. -f1)
SW_VERSION_MINOR            := $(shell git describe --tags --abbrev=0 | cut -c 2- | cut -d. -f2)
SW_VERSION_REVISION         := $(shell git describe --tags --abbrev=0 | cut -c 2- | cut -d. -f3)

GEN_DIR_BASE                := board/gen
GEN_DIR_TARGET_BASE         := $(GEN_DIR_BASE)/target
GEN_DIR_MCU_BASE            := $(GEN_DIR_BASE)/mcu
GEN_DIR_VENDOR_BASE         := $(GEN_DIR_BASE)/vendor
GEN_DIR_ARCH_BASE           := $(GEN_DIR_BASE)/arch
GEN_DIR_TARGET              := $(GEN_DIR_TARGET_BASE)/$(TARGET)
GEN_DIR_TSCREEN_BASE        := application/io/touchscreen/gen

-include $(MAKEFILE_INCLUDE_PREFIX)$(GEN_DIR_TARGET)/Makefile

#these makefiles are specific only to firmware, which is why they don't have MAKEFILE_INCLUDE_PREFIX
#the prefix is used to specify the directory of main, target makefile
#needed for tests since they are outside of src/
-include board/arch/$(ARCH)/$(VENDOR)/Makefile
-include board/arch/$(ARCH)/$(VENDOR)/variants/$(MCU_FAMILY)/Makefile
-include board/arch/$(ARCH)/$(VENDOR)/variants/$(MCU_FAMILY)/$(MCU)/Makefile
-include board/arch/$(ARCH)/Makefile

COMMAND_FW_UPDATE_START     := 0x4F70456E6E45704F
COMMAND_FW_UPDATE_END       := 0x4465436B
SYSEX_MANUFACTURER_ID_0     := 0x00
SYSEX_MANUFACTURER_ID_1     := 0x53
SYSEX_MANUFACTURER_ID_2     := 0x43
FW_METADATA_SIZE            := 4
UART_BAUDRATE_MIDI_STD      := 31250
UART_BAUDRATE_USB           := 38400
UART_BAUDRATE_TOUCHSCREEN   := 38400
ESTA_ID                     := 0x6555

#these can be overriden by target/vendor/arch/mcu etc.
USB_OVER_SERIAL_BUFFER_SIZE ?= 16
I2C_TX_BUFFER_SIZE          ?= 64
UART_TX_BUFFER_SIZE         ?= 128
UART_RX_BUFFER_SIZE         ?= 128
MIDI_SYSEX_ARRAY_SIZE       ?= 100

DEFINES += \
UART_BAUDRATE_MIDI_STD=$(UART_BAUDRATE_MIDI_STD) \
UART_BAUDRATE_USB=$(UART_BAUDRATE_USB) \
UART_BAUDRATE_TOUCHSCREEN=$(UART_BAUDRATE_TOUCHSCREEN) \
USB_OVER_SERIAL_BUFFER_SIZE=$(USB_OVER_SERIAL_BUFFER_SIZE) \
TSCREEN_CDC_PASSTHROUGH_BUFFER_SIZE=$(USB_OVER_SERIAL_BUFFER_SIZE) \
I2C_TX_BUFFER_SIZE=$(I2C_TX_BUFFER_SIZE) \
MIDI_SYSEX_ARRAY_SIZE=$(MIDI_SYSEX_ARRAY_SIZE) \
FIXED_NUM_CONFIGURATIONS=1 \
SYSEX_MANUFACTURER_ID_0=$(SYSEX_MANUFACTURER_ID_0) \
SYSEX_MANUFACTURER_ID_1=$(SYSEX_MANUFACTURER_ID_1) \
SYSEX_MANUFACTURER_ID_2=$(SYSEX_MANUFACTURER_ID_2) \
SW_VERSION_MAJOR=$(SW_VERSION_MAJOR) \
SW_VERSION_MINOR=$(SW_VERSION_MINOR) \
SW_VERSION_REVISION=$(SW_VERSION_REVISION) \
COMMAND_FW_UPDATE_START=$(COMMAND_FW_UPDATE_START) \
COMMAND_FW_UPDATE_END=$(COMMAND_FW_UPDATE_END) \
ESTA_ID=$(ESTA_ID)

ifeq ($(DEBUG), 1)
    DEFINES += DEBUG
endif

DEFINES += UART_TX_BUFFER_SIZE=$(UART_TX_BUFFER_SIZE)
DEFINES += UART_RX_BUFFER_SIZE=$(UART_RX_BUFFER_SIZE)

ifeq ($(TYPE),boot)
    DEFINES += FW_BOOT
    FLASH_START_ADDR := $(BOOT_START_ADDR)
else ifeq ($(TYPE),app)
    DEFINES += FW_APP
    FLASH_START_ADDR := $(APP_START_ADDR)
else ifeq ($(TYPE),flashgen)
    #same as app
    DEFINES += FW_APP
    FLASH_START_ADDR := $(APP_START_ADDR)
    DEFINES := $(filter-out __STM32__,$(DEFINES))
else ifeq ($(TYPE),sysexgen)
    #nothing to do
else
    $(error Invalid firmware type specified)
endif

DEFINES += FLASH_START_ADDR=$(FLASH_START_ADDR)
DEFINES += BOOT_START_ADDR=$(BOOT_START_ADDR)
DEFINES += APP_START_ADDR=$(APP_START_ADDR)
DEFINES += FW_METADATA_LOCATION=$(FW_METADATA_LOCATION)