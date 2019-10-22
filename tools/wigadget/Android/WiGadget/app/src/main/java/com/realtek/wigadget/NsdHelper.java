package com.realtek.wigadget;

import android.content.Context;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.text.format.Formatter;
import android.util.Log;

import org.json.JSONException;

import java.io.IOException;
import java.net.InetAddress;

import javax.jmdns.JmDNS;
import javax.jmdns.ServiceEvent;
import javax.jmdns.ServiceListener;

/**
 * Created by WUJINZHOU on 5/2/15.
 */
public class NsdHelper extends AsyncTask<Void, Void, Void> {
    static final Constants c = Constants.getInstance();

    private final String mServiceType = c.nsd_service_type;
    private final String tag = c.tag_nsd_helper;
    private DeviceList mDeviceList;

    private JmDNS jmdns = null;
    private ServiceListener listener = null;

    private WifiManager wifi = null;
    private WifiManager.MulticastLock multicastLock = null;
    private Context mContext =null;

    public NsdHelper(Context context)throws JSONException,IOException{
        mContext=context;
        mDeviceList=DeviceList.getInstance(mContext);
    }

    @Override
    protected Void doInBackground(Void... params) {
        startNSD();
        return null;
    }

    @Override
    protected void onCancelled() {
        super.onCancelled();
        stopNSD();
    }

    private void startNSD() {
        Log.v(tag, "startNSD() ");

        try {

            wifi = (WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);

            String ip = Formatter.formatIpAddress(wifi.getConnectionInfo().getIpAddress());
            String hostname = InetAddress.getByName(InetAddress.getByName(ip).getHostName()).toString();

            multicastLock = wifi.createMulticastLock(getClass().getName());
            multicastLock.setReferenceCounted(true);
            multicastLock.acquire();

            jmdns = JmDNS.create(InetAddress.getByName(ip),hostname);

            jmdns.addServiceListener(mServiceType, listener = new ServiceListener() {

                @Override
                public void serviceAdded(ServiceEvent event) {
                    jmdns.requestServiceInfo(event.getType(), event.getName(), true);
                    Log.v(tag, "serviceAdded() " + event.getName());
                }

                @Override
                public void serviceRemoved(ServiceEvent event) {
                    Log.v(tag, "serviceRemoved() " + event.getName());
                }

                @Override
                public void serviceResolved(ServiceEvent event) {

                    try {
                        mDeviceList.tempListUpdate(event);
                        String mac = event.getInfo().getPropertyString(c.key_mac);
                        String pairState = event.getInfo().getPropertyString(c.key_pair_state);
                        Log.v(tag, "serviceResolved() " + "mac: " + mac + " pair state: " + pairState);
                    } catch (JSONException e) {
                        Log.e(tag, "serviceResolved() JSONException e" + e);
                    } catch (IOException e) {
                        Log.e(tag, "serviceResolved() IOException e" + e);
                    }
                }

            });

        } catch (IOException e) {
            Log.e(tag,"startNSD() IOException e:"+e);
            e.printStackTrace();
        }
    }

    private void stopNSD() {
        Log.v(tag,"stopNSD() ");
        if (jmdns != null) {
            if (listener != null) {
                jmdns.removeServiceListener(mServiceType, listener);
                listener = null;
            }
            jmdns.unregisterAllServices();
            try {
                jmdns.close();
            } catch (IOException e) {
                Log.e(tag,"stopNSD() IOException e:"+e);
                e.printStackTrace();
            }
            jmdns = null;
        }
        if (multicastLock != null) {
            multicastLock.release();
            multicastLock = null;
        }
    }
}

