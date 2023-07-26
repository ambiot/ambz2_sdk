
For github usage

[GCC]
1. git clone -b amazon https://github.com/ambiot/ambz2_sdk.git
2. git submodule update --init --recursive --depth 1
3. modified lib_amazon/freertos_LTS/vendors/realtek/boards/amebaZ2/aws_demos/config_files/aws_demo_config.h
4. cd project/realtek_amebaz2_v0_example/GCC-RELEASE/
5. make amazon
6. make is

[IAR]
1. git clone -b amazon https://github.com/ambiot/ambz2_sdk.git
2. git submodule update --init --recursive --depth 1
3. modified lib_amazon/freertos_LTS/vendors/realtek/boards/amebaZ2/aws_demos/config_files/aws_demo_config.h
4. open project/realtek_amebaz2_v0_example/EWARM-RELEASEProject_is.eww
5. Build lib_amazon
6. Build application_is

===============================================================================================================================================

7.1d_patch_amaozn_v202012-LTS_20230322_w_v19_(v02).zip

[Description]
    Move flash usage of aws freertos to platform_opts.h and enable CONFIG_EXAMPLE_WLAN_FAST_CONNECT

1.  Notes:
    1. Move flash usage of aws freertos to platform_opts.h
    2. Enable CONFIG_EXAMPLE_WLAN_FAST_CONNECT to handle auto reconnect
    3. Change AUTO_RECONNECT_COUNT to 999

2.  Work with https://github.com/ambiot/amazon-freertos/commits/amebaZ2-7.1d-202107.00-LTS @ 7fcda9a57a

    Clone the amebaZ2 amebaZ2-7.1d-202107.00-LTS repository from our Github (https://github.com/ambiot/amazon-freertos/commits/amebaZ2-7.1d-202107.00-LTS)

    Run the following command under the path: SDK/lib_amazon

    $ git clone --recursive --branch amebaZ2-7.1d-202107.00-LTS --depth 1 https://github.com/ambiot/amazon-freertos.git freertos_LTS

    then you will see all the source code in file: SDK/freertos_LTS/

    $ git checkout 7fcda9a57a9816c83005a0857a5e743277d41a64

    Modified files
        project/realtek_amebaz2_v0_example/inc/platform_opts.h

===============================================================================================================================================

7.1d_patch_amaozn_v202012-LTS_20230322_w_v19_(v01).zip

[Description]
    Support amazon-freertos-202012.00-LTS with MQTT/SHADOWS/OTA demos with IAR and GCC

	The patch is base on sdk-ameba-v7.1d.zip + 7.1d_critical_patch_full_20230303_8d5d1b96_v(19).zip
	
	Please merge the following modified files if use new integrated patch to project, do not replace these files directly, there would be some compile error.
	Please merge the following modified files if use new integrated patch to project, do not replace these files directly, there would be some compile error.

0.	More details for FreeRTOS LTS Libraries
	https://www.freertos.org/lts-libraries.html


1.	Notes:
	1. Support aws_demo (MQTT/SHADOWS/OTA) on AmebaZ2
	2. Replace mbedtls-2.4.0 by amazon mbedtls with LTS version (lib_amazon/freertos_LTS/libraries/3rdparty/mbedtls/library)
	3. Upgrade FreeRTOS kernel to LTS version
	3. Add amazon_ota_tools for generate AWS_OTA firmware

	Modified files
		component/common/api/network/include/lwipopts.h
		component/common/bluetooth/realtek/sdk/board/common/os/freertos/osif_freertos.c
		component/os/freertos/freertos_cb.c
		component/os/freertos/freertos_v10.0.1/Source/portable/GCC/ARM_RTL8710C/portmacro.h
		component/soc/realtek/8710c/cmsis/rtl8710c/source/ram_s/app_start.c		
		project/realtek_amebaz2_v0_example/inc/FreeRTOSConfig_Amazon.h
		project/realtek_amebaz2_v0_example/src/main.c


	Add files
		component/common/example/amazon_freertos/example_amazon_freertos.c
		component/common/example/amazon_freertos/example_amazon_freertos.h
		component/common/example/amazon_freertos/readme.txt
        component/soc/realtek/8710c/fwlib/include/*

		lib_amazon/amazon_ota_tools/*
		lib_amazon/library_version

		project/realtek_amebaz2_v0_example/EWARM-RELEASE/application_is_amazon.ewp
		project/realtek_amebaz2_v0_example/EWARM-RELEASE/lib_amazon.ewp
		project/realtek_amebaz2_v0_example/EWARM-RELEASE/Project_is_amazon.eww
		project/realtek_amebaz2_v0_example/GCC-RELEASE/application.is.mk_amazon
		project/realtek_amebaz2_v0_example/GCC-RELEASE/check_fw_version.sh
		project/realtek_amebaz2_v0_example/GCC-RELEASE/lib_amazon.mk
		project/realtek_amebaz2_v0_example/GCC-RELEASE/Makefile_amazon


	For IAR project
	    In SDK/project/realtek_amebaz2_v0_example/EWARM-RELEASE
		Rename application_is_amazon.ewp -> application_is.ewp
		Rename Project_is_amazon.eww -> Project_is.eww

	For GCC project
	    In SDK/project/realtek_amebaz2_v0_example/GCC-RELEASE
		Rename application.is.mk_amazon -> application.is.mk
		Rename Makefile_amazon -> Makefile



2.  For AWS IoT document, please refer to the Amazon Web
   - https://docs.aws.amazon.com/iot/latest/developerguide/what-is-aws-iot.html



3.  Clone the amebaZ2 amebaZ2-7.1d-202107.00-LTS repository from our Github (https://github.com/ambiot/amazon-freertos/commits/amebaZ2-7.1d-202107.00-LTS)

    Run the following command under the path: SDK/lib_amazon

    $ git clone --recursive --branch amebaZ2-7.1d-202107.00-LTS --depth 1 https://github.com/ambiot/amazon-freertos.git freertos_LTS
	
    then you will see all the source code in file: SDK/lib_amazon/freertos_LTS/



4. Clone the amebaZ2 FreeRTOS-Kernel repository from Github (https://github.com/FreeRTOS/FreeRTOS-Kernel.git)

	Run the following command under the path: SDK/component/os/freertos
	
	git clone --branch V10.4.3-LTS-Patch-2 --depth 1 https://github.com/FreeRTOS/FreeRTOS-Kernel.git freertos_v10.4.3
	
	then you will see all the source code in file: SDK/component/os/freertos/freertos_v10.4.3


   
5.  Configure aws_clientcredential.h and aws_clientcredential_keys.h
	Refer to https://docs.aws.amazon.com/freertos/latest/userguide/freertos-configure.html, which will have the instructions. 

	-In aws_clientcredential.h(SDK/lib_amazon/freertos_LTS/demos/include), set endpoint and wifi info.

		#define clientcredentialMQTT_BROKER_ENDPOINT	"xxxxxxxxxxxxxx.amazonaws.com"
		
		#define clientcredentialIOT_THING_NAME          "xxxxx"

		#define clientcredentialWIFI_SSID				"SSID"
		#define clientcredentialWIFI_PASSWORD			"PASSWORD"

	-In aws_clientcredential_keys.h(SDK/lib_amazon/freertos_LTS/demos/include),set MQTT Demo required credentials
			
		#define keyCLIENT_CERTIFICATE_PEM 			"CERTIFICATE"
		#define keyCLIENT_PRIVATE_KEY_PEM			"PRIVATE_KEY"



6.  For running Amazon SDK example, the following configuration is set.

	-In platform_opts.h

	#define CONFIG_EXAMPLE_AMAZON_FREERTOS		1
	
	- In aws_demo_config.h(SDK/lib_amazon/freertos_LTS/vendors/realtek/boards/amebaZ2/aws_demos/config_files)
	
	//#define CONFIG_CORE_HTTP_MUTUAL_AUTH_DEMO_ENABLED
	#define CONFIG_CORE_MQTT_MUTUAL_AUTH_DEMO_ENABLED
	//#define CONFIG_DEVICE_SHADOW_DEMO_ENABLED
	//#define CONFIG_DEVICE_DEFENDER_DEMO_ENABLED
	//#define CONFIG_OTA_MQTT_UPDATE_DEMO_ENABLED
	//#define CONFIG_OTA_HTTP_UPDATE_DEMO_ENABLED
	//#define CONFIG_JOBS_DEMO_ENABLED
    
	- Default will run CONFIG_CORE_MQTT_MUTUAL_AUTH_DEMO_ENABLED example

	For IAR project
	    In SDK/project/realtek_amebaz2_v0_example/EWARM-RELEASE
		Rename application_is_amazon.ewp -> application_is.ewp
		Rename Project_is_amazon.eww -> Project_is.eww

	For GCC project
	    In SDK/project/realtek_amebaz2_v0_example/GCC-RELEASE
		Rename application.is.mk_amazon -> application.is.mk
		Rename Makefile_amazon -> Makefile



7. Configure ota_demo_config.h for OTA demo
	-In ota_demo_config.h(SDK/vendors/realtek/boards/amebaZ2/aws_demos/config_files/)
		#define otapalconfigCODE_SIGNING_CERTIFICATE \
		"-----BEGIN CERTIFICATE-----\n" \
		"................................................................\n" \
		"................................................................\n" \
		"................................................................\n" \
		"................................................................\n" \
		"................................................................\n" \
		"................................................................\n" \
		"................................................................\n" \
		".........................\n" \
		"-----END CERTIFICATE-----"



8.  OTA update prerequisites (https://docs.aws.amazon.com/freertos/latest/userguide/ota-prereqs.html)
	Step1. Prerequisites for OTA updates using MQTT
	Step2. Create an Amazon S3 bucket to store your update
	Step3. Create an OTA Update service role
	Step4. Create an OTA user policy
	---
	Step5. Create esdsasigner.key and ecdsasigner.crt by openSSL (optional)
		EX:	sudo openssl ecparam -name prime256v1 -genkey -out ecdsa-sha256-signer.key.pem
			sudo openssl req -new -x509 -days 3650 -key ecdsa-sha256-signer.key.pem -out ecdsa-sha256-signer.crt.pem



9.  How is the custom code signing binary created:
	a.	We are using custom signing feature provided by amazon to manually sign the OTA binary(firmware_is_pad.bin).
	b.	The custom signing is being done by means of a python script that we provid in the folder:  SDK/lib_amazon/amazon_ota_tools/python_custom_ecdsa_Z2_iar.py
																									SDK/lib_amazon/amazon_ota_tools/python_custom_ecdsa_Z2_gcc.py
		i.	The python script requires the following pre-requisites to work
			1.	Python must be installed in the windows system with version 3.7.x or later
			2.	Pyopenssl library must be installed using 'pip install pyopenssl'
			3.	The ECDSA signing key and the Certificate pair must be present in the same folder as the python script and must be named 'ecdsa-sha256-signer.key.pem' and 'ecdsa-sha256-signer.crt.pem' respectively.
			
			!!!!!!! The key pair in SDK are just for example, please generated new key by openssl !!!!!!
			!!!!!!! The key pair in SDK are just for example, please generated new key by openssl !!!!!!
			!!!!!!! The key pair in SDK are just for example, please generated new key by openssl !!!!!!
			
			4.	Once all these are present and the python script is run, it will generate a custom signed binary named firmware_is_pad.bin inside the SDK/lib_amazon/amazon_ota_tools/ folder.
                Run the python script in folder: SDK/lib_amazon/amazon_ota_tools/
                *cmd command after IAR build -> $ python python_custom_ecdsa_Z2_iar.py
                *shell command after GCC build -> $ python python_custom_ecdsa_Z2_gcc.py

                There might be some error if there are packages lack in your environment (like openssl...). Please install the package and run the script again.

	c.	After getting the custom signed 'firmware_is_pad.bin' and 'IDT-OTA-Signature', you can upload it to the S3 bucket.



10.  How to trigger a custom signed OTA job in amazon AWS IOT core. 
	a.	Click on 'Create OTA update job', select your thing/things group and then select next.
	b.	In the following page, choose the option 'Use my custom signed firmware image'
	c.	Choose your custom signed firmware binary that was generated by the python script from S3 bucket.
	d.	In the signature field, enter signature 'xxxxxxxxxxxxxxx' in 'IDT-OTA-Signature'
	e.	Choose hash algorithm as 'SHA-256'
	f.	Choose encryption algorithm as 'ECDSA'.
	g.	In "pathname of code signing certificate" and the "Pathname of file on device", both enter '/'
	h.	Choose the IAM role for OTA update job.(This is the same IAM role as any OTA update job)
	i.	Click next, give a unique name for the job and create.
