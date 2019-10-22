##################################################################################
#                                                                                #
#                             Example example_usb_msc                            #
#                                                                                #
##################################################################################

Date: 2018-05-29

Table of Contents
~~~~~~~~~~~~~~~~~
 - Description
 - Setup Guide
 - Parameter Setting and Configuration
 - Result description
 - Other Reference
 - Supported List

 
Description
~~~~~~~~~~~
        With USB device v2.0 interface, Ameba can be designed to a USB mass storage device class(USB MSC).
        In this application, Ameba boots up as USB mass storage and uses SD card as its physical
        memory medium, USB host end (eg,. windows machine) can recognize Ameba and write
        data to and read data from SD card via USB interface.
	
Setup Guide
~~~~~~~~~~~
        In order to run this application successfully, the hardware setup should be confirm before moving on.
        1. Connect Ameba to USB host end with a micro USB cable. Ameba DEV board has
           designed a micro USB connector on board.
        2. Connect SD card to Ameba DEV board
        3. config USB MSC example in platform_opts.h
              #define CONFIG_EXAMPLE_USB_MASS_STORAGE 1
        4. rebuild the project and download firmware to DEV board.

Parameter Setting and Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Step 1, config USB MSC example in platform_opts.h
            #define CONFIG_EXAMPLE_USB_MASS_STORAGE 1
        Step 2, rebuild the project and download firmware to DEV board.
	
Result description
~~~~~~~~~~~~~~~~~~
        On the serial console, USB MSC loading log will be printed, make sure there is no error reported.
        After the MSC driver is successfully loaded, USB host end will
        recognize Ameba as a USB mass storage device. Now user can operate the USB mass
        storage device by adding or deleting file on the memory medium.

Other Reference
~~~~~~~~~~~~~~~
        For more details, please refer to UM0073 Realtek file system storage.pdf

Supported List
~~~~~~~~~~~~~~
[Supported List]
        Supported : Ameba-1
        Source code not in project: Ameba-z, Ameba-pro