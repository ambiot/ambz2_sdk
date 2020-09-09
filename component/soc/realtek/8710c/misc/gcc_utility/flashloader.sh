#!/bin/sh

#===============================================================================
FIRMWARENAME=$1
echo $FIRMWARENAME

#===============================================================================
#get file size
if [ “$(uname)” == “Darwin” ]; then
                FIRMWARE_SIZE=$(stat -f %z $FIRMWARENAME)
else
                FIRMWARE_SIZE=$(stat -c %s $FIRMWARENAME)
fi

echo "set \$FirmwareName = \"$FIRMWARENAME\"" > fwinfo.gdb
echo "set \$FirmwareSize = $FIRMWARE_SIZE" >> fwinfo.gdb

echo "restore $FIRMWARENAME binary (int)&FlashBufferStart-\$Offset \$ReadPtr \$ReadEndPtr" > restore.gdb
exit