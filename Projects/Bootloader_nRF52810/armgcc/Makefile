PROJECT_NAME     := Smart_Remote_3_nRF52_Bootloader_nRF52810
TARGETS          := PCA20031-SR3_nRF52810_Product_Example_Bootloader
OUTPUT_DIRECTORY := _build

SDK_ROOT := ../../../../../..
PROJ_DIR := ../../..

$(OUTPUT_DIRECTORY)/PCA20031-SR3_nRF52810_Product_Example_Bootloader.out: \
  LINKER_SCRIPT  := Smart_Remote_3_nRF52_gcc_nrf52.ld

# Source files common to all targets
SRC_FILES += \
  $(SDK_ROOT)/components/libraries/experimental_log/src/nrf_log_backend_rtt.c \
  $(SDK_ROOT)/components/libraries/experimental_log/src/nrf_log_backend_serial.c \
  $(SDK_ROOT)/components/libraries/experimental_log/src/nrf_log_frontend.c \
  $(SDK_ROOT)/components/libraries/experimental_log/src/nrf_log_str_formatter.c \
  $(SDK_ROOT)/external/nano-pb/pb_common.c \
  $(SDK_ROOT)/external/nano-pb/pb_decode.c \
  $(SDK_ROOT)/components/libraries/util/app_error_weak.c \
  $(SDK_ROOT)/components/libraries/scheduler/app_scheduler.c \
  $(SDK_ROOT)/components/libraries/timer/app_timer.c \
  $(SDK_ROOT)/components/libraries/util/app_util_platform.c \
  $(SDK_ROOT)/components/libraries/crc32/crc32.c \
  $(SDK_ROOT)/components/libraries/hardfault/hardfault_implementation.c \
  $(SDK_ROOT)/components/libraries/hci/hci_mem_pool.c \
  $(SDK_ROOT)/components/libraries/mem_manager/mem_manager.c \
  $(SDK_ROOT)/components/libraries/util/nrf_assert.c \
  $(SDK_ROOT)/components/libraries/atomic_fifo/nrf_atfifo.c \
  $(SDK_ROOT)/components/libraries/balloc/nrf_balloc.c \
  $(SDK_ROOT)/external/fprintf/nrf_fprintf.c \
  $(SDK_ROOT)/external/fprintf/nrf_fprintf_format.c \
  $(SDK_ROOT)/components/libraries/fstorage/nrf_fstorage.c \
  $(SDK_ROOT)/components/libraries/fstorage/nrf_fstorage_nvmc.c \
  $(SDK_ROOT)/components/libraries/fstorage/nrf_fstorage_sd.c \
  $(SDK_ROOT)/components/libraries/experimental_memobj/nrf_memobj.c \
  $(SDK_ROOT)/components/libraries/pwr_mgmt/nrf_pwr_mgmt.c \
  $(SDK_ROOT)/components/libraries/queue/nrf_queue.c \
  $(SDK_ROOT)/components/libraries/experimental_section_vars/nrf_section_iter.c \
  $(SDK_ROOT)/components/libraries/strerror/nrf_strerror.c \
  $(SDK_ROOT)/components/libraries/sha256/sha256.c \
  $(SDK_ROOT)/components/drivers_nrf/clock/nrf_drv_clock.c \
  $(SDK_ROOT)/components/drivers_nrf/common/nrf_drv_common.c \
  $(SDK_ROOT)/components/drivers_nrf/rng/nrf_drv_rng.c \
  $(SDK_ROOT)/components/drivers_nrf/wdt/nrf_drv_wdt.c \
  $(SDK_ROOT)/components/drivers_nrf/hal/nrf_nvmc.c \
  $(SDK_ROOT)/components/softdevice/common/nrf_sdh.c \
  $(SDK_ROOT)/components/softdevice/common/nrf_sdh_ble.c \
  $(SDK_ROOT)/components/softdevice/common/nrf_sdh_soc.c \
  $(SDK_ROOT)/components/libraries/crypto/nrf_crypto_ecdh.c \
  $(SDK_ROOT)/components/libraries/crypto/nrf_crypto_ecdsa.c \
  $(SDK_ROOT)/components/libraries/crypto/nrf_crypto_hash.c \
  $(SDK_ROOT)/components/libraries/crypto/nrf_crypto_init.c \
  $(SDK_ROOT)/components/libraries/crypto/nrf_crypto_keys.c \
  $(SDK_ROOT)/components/libraries/crypto/nrf_crypto_mem.c \
  $(SDK_ROOT)/components/libraries/crypto/nrf_crypto_rng.c \
  $(PROJ_DIR)/Source/Configuration/bootloader_key.c \
  $(PROJ_DIR)/Source/Bootloader/dfu_req_handling/dfu-cc.pb.c \
  $(PROJ_DIR)/Source/Bootloader/dfu_req_handling/dfu_req_handling.c \
  $(PROJ_DIR)/Source/Drivers/drv_board.c \
  $(PROJ_DIR)/Source/Bootloader/sr3_bootloader.c \
  $(SDK_ROOT)/external/segger_rtt/SEGGER_RTT.c \
  $(SDK_ROOT)/external/segger_rtt/SEGGER_RTT_Syscalls_GCC.c \
  $(SDK_ROOT)/external/segger_rtt/SEGGER_RTT_printf.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/micro_ecc/micro_ecc_lib_ecdh.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/micro_ecc/micro_ecc_lib_ecdsa.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/micro_ecc/micro_ecc_lib_init.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/micro_ecc/micro_ecc_lib_keys.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/micro_ecc/micro_ecc_lib_shared.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/nrf_crypto_sw/nrf_crypto_sw_hash.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/nrf_crypto_sw/nrf_crypto_sw_rng.c \
  $(SDK_ROOT)/components/ble/common/ble_srv_common.c \
  $(SDK_ROOT)/components/toolchain/gcc/gcc_startup_nrf52810.S \
  $(SDK_ROOT)/components/toolchain/system_nrf52810.c \
  $(PROJ_DIR)/Source/Bootloader/bootloader/ble_dfu/nrf_ble_dfu.c \
  $(PROJ_DIR)/Source/Bootloader/bootloader/nrf_bootloader.c \
  $(PROJ_DIR)/Source/Bootloader/bootloader/nrf_bootloader_app_start.c \
  $(PROJ_DIR)/Source/Bootloader/bootloader/nrf_bootloader_app_start_asm.c \
  $(PROJ_DIR)/Source/Bootloader/bootloader/nrf_bootloader_info.c \
  $(PROJ_DIR)/Source/Bootloader/bootloader/dfu/nrf_dfu.c \
  $(PROJ_DIR)/Source/Bootloader/bootloader/dfu/nrf_dfu_flash.c \
  $(PROJ_DIR)/Source/Bootloader/bootloader/dfu/nrf_dfu_handling_error.c \
  $(PROJ_DIR)/Source/Bootloader/bootloader/dfu/nrf_dfu_mbr.c \
  $(PROJ_DIR)/Source/Bootloader/bootloader/dfu/nrf_dfu_settings.c \
  $(PROJ_DIR)/Source/Bootloader/bootloader/dfu/nrf_dfu_settings_svci.c \
  $(PROJ_DIR)/Source/Bootloader/bootloader/dfu/nrf_dfu_transport.c \
  $(PROJ_DIR)/Source/Bootloader/bootloader/dfu/nrf_dfu_utils.c \

# Include folders common to all targets
INC_FOLDERS += \
  $(PROJ_DIR)/Source/Configuration \
  $(PROJ_DIR)/Source/Drivers \
  $(SDK_ROOT)/components/libraries/scheduler \
  $(SDK_ROOT)/components/softdevice/s112/headers/nrf52 \
  $(SDK_ROOT)/components/libraries/experimental_log \
  $(SDK_ROOT)/components/libraries/queue \
  $(SDK_ROOT)/components/libraries/pwr_mgmt \
  $(PROJ_DIR)/Source/Bootloader/bootloader/dfu \
  $(SDK_ROOT)/components/libraries/strerror \
  $(SDK_ROOT)/components/libraries/crc32 \
  $(SDK_ROOT)/components/drivers_nrf/delay \
  $(SDK_ROOT)/components/toolchain/cmsis/include \
  $(SDK_ROOT)/components/libraries/mem_manager \
  $(SDK_ROOT)/components/libraries/util \
  $(SDK_ROOT)/components/softdevice/s112/headers \
  $(SDK_ROOT)/components/libraries/crypto \
  $(SDK_ROOT)/components/libraries/balloc \
  $(SDK_ROOT)/components/drivers_nrf/wdt \
  $(SDK_ROOT)/components/libraries/hardfault \
  $(SDK_ROOT)/components/drivers_nrf/rng \
  $(SDK_ROOT)/external/micro-ecc/micro-ecc \
  $(SDK_ROOT)/components/device \
  $(SDK_ROOT)/components/libraries/hci \
  $(PROJ_DIR)/Source/Bootloader/bootloader/ble_dfu \
  $(SDK_ROOT)/components/libraries/fstorage \
  $(SDK_ROOT)/components/libraries/experimental_section_vars \
  $(PROJ_DIR)/Source/Bootloader \
  $(SDK_ROOT)/components/libraries/mutex \
  $(SDK_ROOT)/components/libraries/experimental_log/src \
  $(SDK_ROOT)/external/segger_rtt \
  $(SDK_ROOT)/components/libraries/atomic_fifo \
  ../config \
  $(SDK_ROOT)/components/libraries/atomic \
  $(SDK_ROOT)/components/libraries/sha256 \
  $(SDK_ROOT)/components/libraries/crypto/backend/nrf_crypto_sw \
  $(SDK_ROOT)/components/drivers_nrf/hal \
  $(SDK_ROOT)/components/ble/common \
  $(SDK_ROOT)/components/libraries/experimental_memobj \
  $(SDK_ROOT)/components/toolchain/gcc \
  $(SDK_ROOT)/components/toolchain \
  $(SDK_ROOT)/components/drivers_nrf/common \
  $(SDK_ROOT)/external/fprintf \
  $(SDK_ROOT)/components/drivers_nrf/clock \
  $(SDK_ROOT)/components/softdevice/common \
  $(PROJ_DIR)/Source/Bootloader/dfu_req_handling \
  $(SDK_ROOT)/components/libraries/svc \
  $(SDK_ROOT)/components/libraries/crypto/backend/micro_ecc \
  $(SDK_ROOT)/components/libraries/timer \
  $(PROJ_DIR)/Source/Bootloader/bootloader \
  $(SDK_ROOT)/external/nano-pb \

# Libraries common to all targets
LIB_FILES += \
  $(SDK_ROOT)/external/micro-ecc/nrf52nf_armgcc/armgcc/micro_ecc_lib_nrf52.a \

# Optimization flags
OPT = -Os -g3
# Uncomment the line below to enable link time optimization
#OPT += -flto

# C flags common to all targets
CFLAGS += $(OPT)
CFLAGS += -DBLE_STACK_SUPPORT_REQD
CFLAGS += -DCONFIG_BOARD_NRF52810_PCA20031
CFLAGS += -DDFU_APP_DATA_RESERVED=0x2000
CFLAGS += -DFLASH_BUFFER_COUNT=40
CFLAGS += -DFLASH_BUFFER_LENGTH=128
CFLAGS += -DFLOAT_ABI_SOFT
CFLAGS += -DNRF52810_XXAA
CFLAGS += -DNRF_BOOTLOADER
CFLAGS += -DNRF_DFU_SETTINGS_VERSION=1
CFLAGS += -DNRF_SD_BLE_API_VERSION=5
CFLAGS += -DS112
CFLAGS += -DSOFTDEVICE_PRESENT
CFLAGS += -DSVC_INTERFACE_CALL_AS_NORMAL_FUNCTION
CFLAGS += -DSWI_DISABLE0
CFLAGS += -D__HEAP_SIZE=0
CFLAGS += -mcpu=cortex-m4
CFLAGS += -mthumb -mabi=aapcs
CFLAGS +=  -Wall -Werror
CFLAGS += -mfloat-abi=soft
# keep every function in a separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin -fshort-enums -flto

# C++ flags common to all targets
CXXFLAGS += $(OPT)

# Assembler flags common to all targets
ASMFLAGS += -g3
ASMFLAGS += -mcpu=cortex-m4
ASMFLAGS += -mthumb -mabi=aapcs
ASMFLAGS += -mfloat-abi=soft
ASMFLAGS += -DBLE_STACK_SUPPORT_REQD
ASMFLAGS += -DCONFIG_BOARD_NRF52810_PCA20031
ASMFLAGS += -DDFU_APP_DATA_RESERVED=0x2000
ASMFLAGS += -DFLASH_BUFFER_COUNT=40
ASMFLAGS += -DFLASH_BUFFER_LENGTH=128
ASMFLAGS += -DFLOAT_ABI_SOFT
ASMFLAGS += -DNRF52810_XXAA
ASMFLAGS += -DNRF_BOOTLOADER
ASMFLAGS += -DNRF_DFU_SETTINGS_VERSION=1
ASMFLAGS += -DNRF_SD_BLE_API_VERSION=5
ASMFLAGS += -DS112
ASMFLAGS += -DSOFTDEVICE_PRESENT
ASMFLAGS += -DSVC_INTERFACE_CALL_AS_NORMAL_FUNCTION
ASMFLAGS += -DSWI_DISABLE0
ASMFLAGS += -D__HEAP_SIZE=0
ASMFLAGS += -flto

# Linker flags
LDFLAGS += $(OPT)
LDFLAGS += -mthumb -mabi=aapcs -L $(TEMPLATE_PATH) -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m4
# let linker dump unused sections
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -flto --specs=nano.specs


# Add standard libraries at the very end of the linker input, after all objects
# that may need symbols provided by these libraries.
LIB_FILES += -lc -lnosys -lm

.PHONY: default help

# Build all targets by default
default: all

# Print all targets that can be built
help:
	@echo The following targets are available:
	@echo		PCA20031-SR3_nRF52810_Product_Example_Bootloader
	@echo		sdk_config - starting external tool for editing sdk_config.h
	@echo		flash      - flashing binary

TEMPLATE_PATH := $(SDK_ROOT)/components/toolchain/gcc


include $(TEMPLATE_PATH)/Makefile.common

$(foreach target, $(TARGETS), $(call define_target, $(target)))

.PHONY: flash erase

# Flash the program
flash: $(OUTPUT_DIRECTORY)/PCA20031-SR3_nRF52810_Product_Example_Bootloader.hex
	@echo Flashing: $<
	nrfjprog -f nrf52 --program $< --sectorerase
	nrfjprog -f nrf52 --reset

erase:
	nrfjprog -f nrf52 --eraseall

SDK_CONFIG_FILE := ../config/sdk_config.h
CMSIS_CONFIG_TOOL := $(SDK_ROOT)/external_tools/cmsisconfig/CMSIS_Configuration_Wizard.jar
sdk_config:
	java -jar $(CMSIS_CONFIG_TOOL) $(SDK_CONFIG_FILE)
