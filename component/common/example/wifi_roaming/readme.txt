##################################################################################
#                                                                                #
#                           example_wifi_roaming                                 #
#                                                                                #
##################################################################################

Date: 2019-11-14

Table of Contents
~~~~~~~~~~~~~~~~~
 - Description
 - Setup Guide
 - Parameter Setting and Configuration
 - Result description
 - Supported List

 
Description
~~~~~~~~~~~
        This example realizes wifi roaming among local area network with the same SSID. 

Setup Guide
~~~~~~~~~~~
        1. In platform_opts.h, please set 
		#define CONFIG_EXAMPLE_WLAN_FAST_CONNECT	1		
		#define CONFIG_EXAMPLE_WIFI_ROAMING		1

Parameter Setting and Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Set the RSSI_THRESHOLD according to the WLAN envrionment.

Result description
~~~~~~~~~~~~~~~~~~
	Can make automatical Wi-Fi connection when booting by using wlan fast connect example.
	Set up two AP with same SSID and password. One AP must be able to move easily (eg mobile hotspot).
	Turn on the mobile hotspot first. Do not turn on the second AP.
	Turn the device and connect to the first wifi. After connected, turn on the second AP.
	Move the first AP far from the device. 
	Check the log of the device. It should change connection to the second AP because the second AP has stronger signal.

Supported List
~~~~~~~~~~~~~~
[Supported List]
	Supported :
	    Ameba-1, Ameba-z, Ameba-pro, Ameba-z2, Ameba-D