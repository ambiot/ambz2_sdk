#!/bin/bash

APP_VERSION_MAJOR=`cat ../../../lib_amazon/freertos_LTS/vendors/realtek/boards/amebaZ2/aws_demos/config_files/ota_demo_config.h | grep -i define | grep -i APP_VERSION_MAJOR | cut -d " " -f 10 `
JSON_VERSION=`cat amebaz2_firmware_is.json | grep -i serial | sed -n 1p | cut -d ":" -f 2 | sed "s/^[ \t]*//g"`

if [ "${APP_VERSION_MAJOR}" != "${JSON_VERSION}" ]
then
	echo "APP_VERSION_MAJOR  = $APP_VERSION_MAJOR"
	echo "serial = $JSON_VERSION"
	echo "!!! Please check FW version setting in \"ota_demo_config.h\" and \"amebaz2_firmware_is.json\". The version must same !!!"
	echo "!!! Please check FW version setting in \"ota_demo_config.h\" and \"amebaz2_firmware_is.json\". The version must same !!!"
	exit 1
fi
