package com.realtek.wigadget;

import android.graphics.Bitmap;

public class DeviceIcon {
    private Bitmap deviceIcon;
    private String deviceMac;
    private String deviceDescription;

    public DeviceIcon(Bitmap icon, String description, String mac) {
        super();
        this.deviceIcon = icon;
        this.deviceDescription=description;
        this.deviceMac = mac;
    }

    public Bitmap getDeviceIcon() {
        return deviceIcon;
    }

    public String getDeviceDescription() {
        return deviceDescription;
    }

    public void setDeviceDescription(String description) {
        this.deviceDescription = description;
    }

    public String getDeviceMac(){
        return deviceMac;
    }

}
