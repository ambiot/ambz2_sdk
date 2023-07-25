
# Initialize tool chain
# -------------------------------------------------------------------

AMEBAZ2_TOOLDIR	= ../../../component/soc/realtek/8710c/misc/iar_utility

OS := $(shell uname)

CROSS_COMPILE = $(ARM_GCC_TOOLCHAIN)/arm-none-eabi-

# Compilation tools
AR = $(CROSS_COMPILE)ar
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
NM = $(CROSS_COMPILE)nm
LD = $(CROSS_COMPILE)gcc
GDB = $(CROSS_COMPILE)gdb
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

OS := $(shell uname)

# Initialize target name and target object files
# -------------------------------------------------------------------

all: lib_amazon

TARGET=lib_amazon

OBJ_DIR=$(TARGET)/Debug/obj
BIN_DIR=$(TARGET)/Debug/bin
INFO_DIR=$(TARGET)/Debug/info

# Include folder list
# -------------------------------------------------------------------

INCLUDES =
INCLUDES += -I../inc

INCLUDES += -I../../../component/common/api
INCLUDES += -I../../../component/common/api/platform
INCLUDES += -I../../../component/common/api/wifi
INCLUDES += -I../../../component/common/api/wifi/rtw_wpa_supplicant/src
INCLUDES += -I../../../component/common/api/wifi/rtw_wpa_supplicant/src/crypto
INCLUDES += -I../../../component/common/api/network/include
INCLUDES += -I../../../component/common/application
INCLUDES += -I../../../component/common/application/mqtt/MQTTClient
INCLUDES += -I../../../component/common/example
INCLUDES += -I../../../component/common/utilities
INCLUDES += -I../../../component/common/mbed/hal
INCLUDES += -I../../../component/common/mbed/hal_ext
INCLUDES += -I../../../component/common/mbed/targets/hal/rtl8710c
INCLUDES += -I../../../component/common/network
INCLUDES += -I../../../component/common/network/lwip/lwip_v2.0.2/src/include
INCLUDES += -I../../../component/common/network/lwip/lwip_v2.0.2/src/include/lwip
INCLUDES += -I../../../component/common/network/lwip/lwip_v2.0.2/port/realtek
INCLUDES += -I../../../component/common/network/lwip/lwip_v2.0.2/port/realtek/freertos
#INCLUDES += -I../../../component/common/network/ssl/mbedtls-2.4.0/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/3rdparty/mbedtls/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/3rdparty/mbedtls_config
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/3rdparty/mbedtls_utils
#aws_mbedtls_config.h
INCLUDES += -I../../../lib_amazon/
INCLUDES += -I../../../component/common/network/ssl/ssl_ram_map/rom
INCLUDES += -I../../../component/common/drivers/wlan/realtek/include
INCLUDES += -I../../../component/common/drivers/wlan/realtek/src/osdep
INCLUDES += -I../../../component/common/drivers/wlan/realtek/src/core/option
INCLUDES += -I../../../component/common/test
INCLUDES += -I../../../component/soc/realtek/8710c/cmsis/rtl8710c/include
INCLUDES += -I../../../component/soc/realtek/8710c/cmsis/rtl8710c/lib/include
INCLUDES += -I../../../component/soc/realtek/8710c/fwlib/include
INCLUDES += -I../../../component/soc/realtek/8710c/fwlib/lib/include
INCLUDES += -I../../../component/soc/realtek/8710c/cmsis/cmsis-core/include
INCLUDES += -I../../../component/soc/realtek/8710c/app/rtl_printf/include
INCLUDES += -I../../../component/soc/realtek/8710c/app/shell
INCLUDES += -I../../../component/soc/realtek/8710c/app/stdio_port
INCLUDES += -I../../../component/soc/realtek/8710c/misc/utilities/include
INCLUDES += -I../../../component/soc/realtek/8710c/mbed-drivers/include
INCLUDES += -I../../../component/soc/realtek/8710c/misc/platform
INCLUDES += -I../../../component/soc/realtek/8710c/misc/driver
INCLUDES += -I../../../component/soc/realtek/8710c/misc/os

INCLUDES += -I../../../component/os/freertos
#INCLUDES += -I../../../component/os/freertos/freertos_v10.0.1/Source/include
#INCLUDES += -I../../../component/os/freertos/freertos_v10.0.1/Source/portable/GCC/ARM_RTL8710C
##os - freertos 10.4.3 start
INCLUDES += -I../../../component/os/freertos/freertos_v10.4.3/include
INCLUDES += -I../../../component/os/freertos/freertos_v10.4.3/portable/GCC/ARM_CM33_NTZ/non_secure
#os - freertos 10.4.3 end
INCLUDES += -I../../../component/os/os_dep/include

#Amazon Includes
INCLUDES += -I../../../component/common/example/amazon_freertos

INCLUDES += -I../../../lib_amazon/freertos_LTS/demos/device_defender_for_aws/
INCLUDES += -I../../../lib_amazon/freertos_LTS/demos/dev_mode_key_provisioning/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/demos/common/pkcs11_helpers
INCLUDES += -I../../../lib_amazon/freertos_LTS/demos/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/demos/network_manager
INCLUDES += -I../../../lib_amazon/freertos_LTS/demos/common/http_demo_helpers
INCLUDES += -I../../../lib_amazon/freertos_LTS/demos/common/mqtt_demo_helpers
INCLUDES += -I../../../lib_amazon/freertos_LTS/demos/dev_mode_key_provisioning/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/demos/common/pkcs11_helpers
INCLUDES += -I../../../lib_amazon/freertos_LTS/demos/device_defender_for_aws
INCLUDES += -I../../../lib_amazon/freertos_LTS/demos/common/mqtt_subscription_manager
INCLUDES += -I../../../lib_amazon/freertos_LTS/demos/fleet_provisioning_with_csr

INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/freertos_plus/standard/utils/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/logging/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/abstractions/wifi/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/3rdparty/unity/src
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/3rdparty/unity/extras/fixture/src
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/c_sdk/standard/common/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/abstractions/platform/freertos/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/abstractions/platform/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/abstractions/secure_sockets/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/c_sdk/standard/common/include/private
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/abstractions/pkcs11/corePKCS11/source/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/abstractions/pkcs11/corePKCS11/source/dependency/3rdparty/pkcs11
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/c_sdk/standard/common/include/private
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/c_sdk/standard/common/include/private
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/abstractions/backoff_algorithm/source/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/coreHTTP/source/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/coreHTTP/source/interface
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/abstractions/transport/secure_sockets
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/coreMQTT/source/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/3rdparty/jsmn
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/freertos_plus/aws/ota/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/3rdparty/mbedtls_utils
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/freertos_plus/standard/crypto/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/c_sdk/aws/defender/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/c_sdk/standard/mqtt/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/c_sdk/standard/serializer/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/c_sdk/aws/shadow/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/c_sdk/standard/mqtt/test/access
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/coreHTTP/source/dependency/3rdparty/http_parser
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/c_sdk/standard/https/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/c_sdk/standard/https/test/access
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/c_sdk/standard/mqtt/src
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/c_sdk/standard/mqtt/test/mock
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/3rdparty/tinycbor/src
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/freertos_plus/aws/ota/src
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/freertos_plus/standard/tls/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/device_shadow_for_aws/source/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/coreJSON/source/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/freertos_plus/aws/ota/test
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/jobs_for_aws/source/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/device_defender_for_aws/source/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/ota_for_aws/source/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/ota_for_aws/source
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/ota_for_aws/source/portable
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/ota_for_aws/source/portable/os
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/abstractions/mqtt_agent/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/coreMQTT-Agent/source/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/fleet-provisioning/source/include
INCLUDES += -I../../../lib_amazon/freertos_LTS/libraries/abstractions/platform/include/platform

INCLUDES += -I../../../lib_amazon/freertos_LTS/vendors/realtek/boards/amebaZ2/aws_demos/config_files
INCLUDES += -I../../../lib_amazon/freertos_LTS/vendors/realtek/boards/amebaZ2/ports/ota

# Source file list
# -------------------------------------------------------------------

SRC_C =

###libraries
##3rdparty
#jsmn
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/3rdparty/jsmn/jsmn.c
#mbedtls_utils
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/3rdparty/mbedtls_utils/mbedtls_error.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/3rdparty/mbedtls_utils/mbedtls_utils.c
#tinycbor
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/3rdparty/tinycbor/src/cborencoder.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/3rdparty/tinycbor/src/cborencoder_close_container_checked.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/3rdparty/tinycbor/src/cborerrorstrings.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/3rdparty/tinycbor/src/cborparser.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/3rdparty/tinycbor/src/cborparser_dup_string.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/3rdparty/tinycbor/src/cborpretty.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/3rdparty/tinycbor/src/cborpretty_stdio.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/3rdparty/tinycbor/src/cborvalidation.c
##abstractions
#backoff_algorithm
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/abstractions/backoff_algorithm/source/backoff_algorithm.c
#mqtt_agent
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/abstractions/mqtt_agent/freertos_agent_message.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/abstractions/mqtt_agent/freertos_command_pool.c
#pkcs11
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/abstractions/pkcs11/corePKCS11/source/portable/mbedtls/core_pkcs11_mbedtls.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/abstractions/pkcs11/corePKCS11/source/core_pkcs11.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/abstractions/pkcs11/corePKCS11/source/core_pki_utils.c
#platform - freertos
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/abstractions/platform/freertos/iot_clock_freertos.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/abstractions/platform/freertos/iot_metrics.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/abstractions/platform/freertos/iot_network_freertos.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/abstractions/platform/freertos/iot_threads_freertos.c
#transport - secure_sockets
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/abstractions/transport/secure_sockets/transport_secure_sockets.c
##c_sdk
#standard
#common - taskpool
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/c_sdk/standard/common/taskpool/iot_taskpool.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/c_sdk/standard/common/taskpool/iot_taskpool_static_memory.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/c_sdk/standard/common/iot_device_metrics.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/c_sdk/standard/common/iot_init.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/c_sdk/standard/common/iot_static_memory_common.c
##coreHTTP
#http_parser
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/coreHTTP/source/dependency/3rdparty/http_parser/http_parser.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/coreHTTP/source/core_http_client.c
##coreJSON
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/coreJSON/source/core_json.c
##coreMQTT
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/coreMQTT/source/core_mqtt.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/coreMQTT/source/core_mqtt_serializer.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/coreMQTT/source/core_mqtt_state.c
##coreMQTT-Agent
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/coreMQTT-Agent/source/core_mqtt_agent.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/coreMQTT-Agent/source/core_mqtt_agent_command_functions.c
##device_shadow_for_aws
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/device_shadow_for_aws/source/shadow.c
##device_defender_for_aws
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/device_defender_for_aws/source/defender.c
##freertos_plus
#standard
#crypto
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/freertos_plus/standard/crypto/src/iot_crypto.c
#tls
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/freertos_plus/standard/tls/src/iot_tls.c
#utils
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/freertos_plus/standard/utils/src/iot_system_init.c
##jobs_for_aws
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/jobs_for_aws/source/jobs.c
##logging
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/logging/iot_logging.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/logging/iot_logging_task_dynamic_buffers.c
##ota_for_aws
#os
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/ota_for_aws/source/portable/os/ota_os_freertos.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/ota_for_aws/source/ota.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/ota_for_aws/source/ota_base64.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/ota_for_aws/source/ota_cbor.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/ota_for_aws/source/ota_http.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/ota_for_aws/source/ota_interface.c
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/ota_for_aws/source/ota_mqtt.c

#fleet-provisioning
SRC_C += ../../../lib_amazon/freertos_LTS/libraries/fleet-provisioning/source/fleet_provisioning.c

###vendor
##port
SRC_C += ../../../lib_amazon/freertos_LTS/vendors/realtek/boards/amebaZ2/ports/ota/aws_ota_amebaZ2.c
SRC_C += ../../../lib_amazon/freertos_LTS/vendors/realtek/boards/amebaZ2/ports/ota/ota_pal.c
SRC_C += ../../../lib_amazon/freertos_LTS/vendors/realtek/boards/amebaZ2/ports/pkcs11/core_pkcs11_pal.c
SRC_C += ../../../lib_amazon/freertos_LTS/vendors/realtek/boards/amebaZ2/ports/secure_sockets/iot_secure_sockets.c
SRC_C += ../../../lib_amazon/freertos_LTS/vendors/realtek/boards/amebaZ2/ports/wifi/iot_wifi.c

##amazon_freertos_LTS - demos
#common
SRC_C += ../../../lib_amazon/freertos_LTS/demos/common/http_demo_helpers/http_demo_utils.c
SRC_C += ../../../lib_amazon/freertos_LTS/demos/common/mqtt_demo_helpers/mqtt_demo_helpers.c
SRC_C += ../../../lib_amazon/freertos_LTS/demos/common/mqtt_subscription_manager/mqtt_subscription_manager.c
SRC_C += ../../../lib_amazon/freertos_LTS/demos/common/ota_demo_helpers/ota_application_version.c
SRC_C += ../../../lib_amazon/freertos_LTS/demos/common/pkcs11_helpers/pkcs11_helpers.c
#coreHTTP
SRC_C += ../../../lib_amazon/freertos_LTS/demos/coreHTTP/http_demo_mutual_auth.c
#coreMQTT
SRC_C += ../../../lib_amazon/freertos_LTS/demos/coreMQTT/mqtt_demo_mutual_auth.c
#coreMQTT_Agent
SRC_C += ../../../lib_amazon/freertos_LTS/demos/coreMQTT_Agent/mqtt_agent_task.c
SRC_C += ../../../lib_amazon/freertos_LTS/demos/coreMQTT_Agent/simple_sub_pub_demo.c
SRC_C += ../../../lib_amazon/freertos_LTS/demos/coreMQTT_Agent/subscription_manager.c
#demo_runner
SRC_C += ../../../lib_amazon/freertos_LTS/demos/demo_runner/aws_demo.c
SRC_C += ../../../lib_amazon/freertos_LTS/demos/demo_runner/iot_demo_freertos.c
SRC_C += ../../../lib_amazon/freertos_LTS/demos/demo_runner/iot_demo_runner.c
#dev_mode_key_provisioning
SRC_C += ../../../lib_amazon/freertos_LTS/demos/dev_mode_key_provisioning/src/aws_dev_mode_key_provisioning.c
#device_shadow_for_aws
SRC_C += ../../../lib_amazon/freertos_LTS/demos/device_shadow_for_aws/shadow_demo_main.c
#device_defender_for_aws
SRC_C += ../../../lib_amazon/freertos_LTS/demos/device_defender_for_aws/metrics_collector/lwip/metrics_collector.c
SRC_C += ../../../lib_amazon/freertos_LTS/demos/device_defender_for_aws/defender_demo.c
SRC_C += ../../../lib_amazon/freertos_LTS/demos/device_defender_for_aws/report_builder.c
#jobs_for_aws
SRC_C += ../../../lib_amazon/freertos_LTS/demos/jobs_for_aws/jobs_demo.c
#network_manager
SRC_C += ../../../lib_amazon/freertos_LTS/demos/network_manager/aws_iot_network_manager.c
#ota
SRC_C += ../../../lib_amazon/freertos_LTS/demos/ota/ota_demo_core_mqtt/ota_demo_core_mqtt.c
SRC_C += ../../../lib_amazon/freertos_LTS/demos/ota/ota_demo_core_http/ota_demo_core_http.c

SRC_C += ../../../lib_amazon/freertos_LTS/vendors/realtek/boards/amebaZ2/aws_demos/application_code/aws_main.c

#fleet_provisioning_with_csr
SRC_C += ../../../lib_amazon/freertos_LTS/demos/fleet_provisioning_with_csr/fleet_provisioning_demo.c
SRC_C += ../../../lib_amazon/freertos_LTS/demos/fleet_provisioning_with_csr/pkcs11_operations.c
SRC_C += ../../../lib_amazon/freertos_LTS/demos/fleet_provisioning_with_csr/tinycbor_serializer.c

#lib_version
VER_C += $(TARGET)_version.c

# Generate obj list
# -------------------------------------------------------------------

SRC_O = $(patsubst %.c,%_$(TARGET).o,$(SRC_C))
VER_O = $(patsubst %.c,%_$(TARGET).o,$(VER_C))

SRC_C_LIST = $(notdir $(SRC_C)) $(notdir $(DRAM_C))
OBJ_LIST = $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%_$(TARGET).o,$(SRC_C_LIST)))
DEPENDENCY_LIST = $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%_$(TARGET).d,$(SRC_C_LIST)))

# Compile options
# -------------------------------------------------------------------

CFLAGS =
CFLAGS += -march=armv8-m.main+dsp -mthumb -mcmse -mfloat-abi=soft -D__thumb2__ -g -gdwarf-3 -Os
CFLAGS += -D__ARM_ARCH_8M_MAIN__=1 -gdwarf-3 -fstack-usage -fdata-sections -ffunction-sections 
CFLAGS += -fdiagnostics-color=always -Wall -Wpointer-arith -Wstrict-prototypes -Wundef -Wno-write-strings 
CFLAGS += -Wno-maybe-uninitialized --save-temps -c -MMD
CFLAGS += -DCONFIG_PLATFORM_8710C -DCONFIG_BUILD_RAM=1
CFLAGS += -DV8M_STKOVF
CFLAGS += -DENABLE_AMAZON_COMMON
CFLAGS += -DMBEDTLS_CONFIG_FILE=\"aws_mbedtls_config.h\"

include toolchain.mk

# Compile
# -------------------------------------------------------------------

.PHONY: lib_amazon
lib_amazon: prerequirement $(SRC_O) $(DRAM_O)
	$(AR) crv $(BIN_DIR)/$(TARGET).a $(OBJ_CPP_LIST) $(OBJ_LIST) $(VER_O)
	cp $(BIN_DIR)/$(TARGET).a ../../../component/soc/realtek/8710c/misc/bsp/lib/common/GCC/$(TARGET).a

# Manipulate Image
# -------------------------------------------------------------------

.PHONY: manipulate_images
manipulate_images:
	@echo ===========================================================
	@echo Image manipulating
	@echo ===========================================================

# Generate build info
# -------------------------------------------------------------------

.PHONY: prerequirement
prerequirement:
	@rm -f $(TARGET)_version*.o
	@echo const char $(TARGET)_rev[] = \"$(TARGET)_ver_`git rev-parse HEAD`_`date +%Y/%m/%d-%T`\"\; > $(TARGET)_version.c
	@$(CC) $(CFLAGS) $(INCLUDES) -c $(VER_C) -o $(VER_O)
	@if [ ! -d $(ARM_GCC_TOOLCHAIN) ]; then \
		echo ===========================================================; \
		echo Toolchain not found, \"make toolchain\" first!; \
		echo ===========================================================; \
		exit -1; \
	fi
	@echo ===========================================================
	@echo Build $(TARGET)
	@echo ===========================================================
	mkdir -p $(OBJ_DIR)
	mkdir -p $(BIN_DIR)
	mkdir -p $(INFO_DIR)

$(SRC_O): %_$(TARGET).o : %.c | prerequirement
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -MM -MT $@ -MF $(OBJ_DIR)/$(notdir $(patsubst %.o,%.d,$@))
	cp $@ $(OBJ_DIR)/$(notdir $@)
	mv $(notdir $*.i) $(INFO_DIR)
	mv $(notdir $*.s) $(INFO_DIR)
	chmod 777 $(OBJ_DIR)/$(notdir $@)

-include $(DEPENDENCY_LIST)

.PHONY: clean
clean:
	rm -rf $(TARGET)
	rm -f $(SRC_O) $(DRAM_O) $(VER_O)
	rm -f $(patsubst %.o,%.d,$(SRC_O)) $(patsubst %.o,%.d,$(DRAM_O)) $(patsubst %.o,%.d,$(VER_O))
	rm -f $(patsubst %.o,%.su,$(SRC_O)) $(patsubst %.o,%.su,$(DRAM_O)) $(patsubst %.o,%.su,$(VER_O))
	rm -f *.i
	rm -f *.s
	rm -f $(VER_C)
