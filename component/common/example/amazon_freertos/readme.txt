AMAZON FREERTOS SDK EXAMPLE

Description:
Start to run Amazon FreeRTOS SDK on Ameba

Configuration:

1. Configure aws_clientcredential.h and aws_clientcredential_keys.h
Refer to https://docs.aws.amazon.com/freertos/latest/userguide/freertos-configure.html, which will have the instructions. 

2.
	For Ameba Z2
		For IgnoreSecure project (IAR):

			In IAR, change the workspace to "amazon_freertos"

			-In platform_opts.h,
			
				#define CONFIG_SSL_CLIENT_PRIVATE_IN_TZ		0
				#define CONFIG_EXAMPLE_AMAZON_FREERTOS		1
				
			-In aws_clientcredential.h(..component\common\application\amazon\amazon-freertos-1.4.7\demos\common\include), set network connection related info
			
				#define clientcredentialWIFI_SSID			"SSID"
				#define clientcredentialWIFI_PASSWORD		"PASSWORD"
				
			-In aws_clientcredential_keys.h(..component\common\application\amazon\amazon-freertos-1.4.7\demos\common\include),, set MQTT Demo required credentials
			
				#define keyCLIENT_CERTIFICATE_PEM 			"CERTIFICATE"
				#define keyCLIENT_PRIVATE_KEY_PEM			"PRIVATE_KEY"
			
		For IgnoreSecure project (GCC):
		
			Configurations are the same. Need to rename "application.is.amazon.mk" to "application.is.mk", and replace it with the original "application.is.mk".
			
		For TrustZone project (IAR):

			If use both secure and non-secure memory
			 
				In IAR, change the workspace to "application_ns-amazon_freertos"

				-In platform_opts.h,
				
					#define CONFIG_SSL_CLIENT_PRIVATE_IN_TZ		1
					#define CONFIG_EXAMPLE_AMAZON_FREERTOS		1
				 
				-In aws_clientcredential.h(..component\common\application\amazon\amazon-freertos-1.4.7\demos\common\include),, set network connection related info
				
					#define clientcredentialWIFI_SSID			"SSID"
					#define clientcredentialWIFI_PASSWORD		"PASSWORD"
					
				-In aws_clientcredential_keys.h(..component\common\application\amazon\amazon-freertos-1.4.7\demos\common\include),, set MQTT Demo required certificate
				
					#define keyCLIENT_CERTIFICATE_PEM 			"CERTIFICATE"
				
				-In main_s.c, set MQTT Demo required private key
				
					const char *client_key_s = \
					"-----BEGIN EC PARAMETERS-----\r\n" \
					"BggqhkjOPQMBBw==\r\n" \
					"-----END EC PARAMETERS-----\r\n" \
					"PRIVATE_KEY";
					
			If only use non-secure memory
			
				In IAR, change the workspace to "application_ns-amazon_freertos"

				-In platform_opts.h,
				
					#define CONFIG_SSL_CLIENT_PRIVATE_IN_TZ		0
					#define CONFIG_EXAMPLE_AMAZON_FREERTOS		1
				 
				-In aws_clientcredential.h(..component\common\application\amazon\amazon-freertos-1.4.7\demos\common\include),, set network connection related info
			
				#define clientcredentialWIFI_SSID			"SSID"
				#define clientcredentialWIFI_PASSWORD		"PASSWORD"
				
				-In aws_clientcredential_keys.h(..component\common\application\amazon\amazon-freertos-1.4.7\demos\common\include),, set MQTT Demo required credentials
			
				#define keyCLIENT_CERTIFICATE_PEM 			"CERTIFICATE"
				#define keyCLIENT_PRIVATE_KEY_PEM			"PRIVATE_KEY"
				
				Logging service will be turned off
				
		For TrustZone project (GCC):
		
			Configurations are the same. Need to rename "application.ns.amazon.mk" to "application.ns.mk", and replace it with the original "application.ns.mk".

Execution:
The example will run demos defined in aws_demo_runner.c.
