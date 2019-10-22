#!/bin/sh

#===============================================================================
FIRMWARENAME=$1
echo $FIRMWARENAME

#===============================================================================
#get file size
FIRMWARE_SIZE=$(stat -c %s $FIRMWARENAME)

echo "set \$FirmwareName = \"$FIRMWARENAME\"" > fwinfo.gdb
echo "set \$FirmwareSize = $FIRMWARE_SIZE" >> fwinfo.gdb

echo "restore $FIRMWARENAME binary (int)&FlashBufferStart-\$Offset \$ReadPtr \$ReadEndPtr" > restore.gdb
exit