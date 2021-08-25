[General]
You can use tickless roaming when you enable tickless.

[Preconditions]
Please configure these options and recompile the project
1.add the "example_tickless_wifi_roaming.c" to the project
2.#define CONFIG_EXAMPLE_WLAN_FAST_CONNECT	1
3.#define CONFIG_EXAMPLE_TICKLESS_WIFI_ROAMING 1
In FreeRTOSConfig.h
4.#define configUSE_TICKLESS_IDLE 1
5.#define configUSE_CUSTOMIZED_TICKLESS_IDLE 1

[Procedure]
1.) Setup two AP with the same SSID and password, hereby named as "first" and "second" AP for
    references
2.) Boot up AmebaZ2 near to the "first AP". After connection to first AP succeed, and idle for 
    some time, AmebaZ2 will enter into tickless mode.
    Printed message("tickless roaming")
3.) Move AmebaZ2 away from the first AP and if the RSSI drop below RSSI_SCAN_THRESHOLD(-40),
    AmebaZ2 will start scanning for better AP.
    Printed message("Start scan, current rssi(XX) < scan threshold rssi(XX)")
4.) Move AmebaZ2 close to "second AP". If an AP with better RSSI is found, 
    it will connect to it.
    Printed message("The found ap IS Better, rssi(XX)")
    Printed message("Start roaming, current rssi(XX) < threshold(XX),target ap(XX)")

[Supported List]
	Supported :
	    Ameba-Z, Ameba-Z2, Ameba-D, 
	Source code not in project:
	    
