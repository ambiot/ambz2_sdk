Google Seamless BLE Setup EXAMPLE

Description:
Perform as BLE peripheral for a Google Nest or Google Home speaker or display 

Configuration:
[platform_opts_bt.h]
	#define CONFIG_BT						1
	#define CONFIG_BT_GOOGLE_SEAMLESS				1
For IAR build, please include build all the module under application_is/bluetooth/example/ble_google_seamless
For GCC build, please remove the comment for module in application.is.mk under
#bluetooth - example - ble_google_seamless

Execution:
When define CONFIG_BT 1 and define GOOGLE_SEAMLESS_EXAMPLE  1,
By command "ATBH=1", once see "GAP adv start" on the console, you can start 
using Google Smart Home Application to connect to Ameba.

Note:
1."ATBH=1", Ameba start send broadcasting ADV packest.

2. Ameba stops broadcasting ADV packest when BT connection established 

3. After the connection is successful, and the ameba board is added and shown
in the Google Home App. Speak "Turn on xx(depends on the type of device you
 configured in Google local fulfillment)" or "Turn off xx". Google Speaker will 
send the "on" or "off"command to Ameba and write to GATT_UUID_CHAR_LIGHT_ON_STATE.
After write successfully, the notification package will be send back to Google speaker 
to feedback the current status.

4. For the auto reconnection function, when the device is added into google home application.
The status of device change from un-provision to provisioned. If there is discounnection happen, 
such as ATBH=0, then re-establish the connection by ATBH=1. The device will be controllable again 
from the application. You don't need to re-add the new device to the Google Home application. 

5. If you want to save the provisioned state when there is power shut down, please 
allocated the flash memory to store the provisioned state. This example is use ATBH=0 to simulate 
power shut down during run time. For this example, we are using ram variable which at google_seamless.c
bool google_seamless_provisioned = false; 

6. When the device change to provision status, the advertisement package content will be different
compare to the unprovision device, we will update the advertisement package information at 5th byte
 and 6th byte before next advertisement start. For the 5th and 6th byte location is used for this
 example only, if there is change for this structure, the byte location is required to be updated accordingly. 
The provison and unprovision advertisement package information have been saved in the Google action
 console for Google Speaker to identify whether your device has been provision or added to the 
application before. 

[Supported List]
	Supported :
	    Ameba-z2