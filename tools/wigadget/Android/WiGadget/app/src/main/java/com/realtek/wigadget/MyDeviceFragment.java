package com.realtek.wigadget;

import android.app.AlertDialog;
import android.app.Fragment;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.EditText;
import android.widget.GridView;
import android.widget.Toast;

import org.json.JSONException;

import java.io.IOException;
import java.net.InetAddress;
import java.util.ArrayList;

public class MyDeviceFragment extends Fragment {
	
	public MyDeviceFragment(){}

    private GridView gridView;
    private GridViewAdapter gridAdapter;

    private DeviceList mDeviceList;

    private static final Constants c = Constants.getInstance();

    private Handler mHandler;

    private ConnectionChecker connectionChecker;

    private AES128 mAES = new AES128();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(R.string.my_device_fragment_title);
    }

	@Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        final View rootView = inflater.inflate(R.layout.fragment_my_device, container, false);

        try{
            mDeviceList = DeviceList.getInstance(getActivity().getApplicationContext());
        }
        catch (JSONException e){
            Log.e(c.tag_my_device_fragment,"JSONException e --"+e);
        }
        catch (IOException e){
            Log.e(c.tag_my_device_fragment,"IOException e --"+e);
        }

        /////////////////////////////////set gridview/////////////////////////////////////////

        gridView = (GridView) rootView.findViewById(R.id.my_device_gridView);
        try {
            gridAdapter = new GridViewAdapter(getActivity(), R.layout.layout_grid_view_item, getDeviceIcons());
        }
        catch (JSONException e){
            Log.e(c.tag_my_device_fragment,"JSONException e --"+e);
        }

        gridView.setAdapter(gridAdapter);
        gridView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View v, int position, long id) {

                //get mac addr from position:
                String mac = gridAdapter.getDeviceIconAt(position).getDeviceMac();
                Log.v(c.tag_my_device_fragment,"clicked on device :"+mac);

                try {
                    startAmebaActivity(mac);
                }
                catch (JSONException e){
                    Log.e(c.tag_my_device_fragment,"startAmebaActivity() JSONException e "+e);
                }
            }
        });

        gridView.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
            @Override
            public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {

                popupOptions(id);

                return true;
            }
        });
        /////////////////////////////////set gridview/////////////////////////////////////////

        /////////////////////////////////update ui thread/////////////////////////////////////
        mHandler=new Handler();
        startUpdateUi();
        /////////////////////////////////update ui thread/////////////////////////////////////

        return rootView;
    }


    private void popupOptions(long id){

        final long idx = id;
        final ImageListAdapter adapter = new ImageListAdapter (getView().getContext(),getResources().getStringArray(R.array.my_device_popup_dialog_options));

        final AlertDialog.Builder builder = new AlertDialog.Builder(getView().getContext());

        builder.setTitle(R.string.popup_dialog_title_options);
        builder.setCancelable(true);

        builder.setAdapter(adapter, new DialogInterface.OnClickListener() {

            @Override
            public void onClick(DialogInterface dialog, int index) {

                ///////////////switch controls/////////////////
                if (index == 0) {

                    LayoutInflater inflater;
                    View promptsView;
                    AlertDialog.Builder dialogBuilder;
                    final EditText etUserInput;
                    final String[] newDescription = {""};

                    inflater = LayoutInflater.from(getActivity());
                    promptsView = inflater.inflate(R.layout.layout_popup_dialog_prompts_rename, null);
                    dialogBuilder = new AlertDialog.Builder(getActivity());
                    dialogBuilder.setView(promptsView);
                    etUserInput = (EditText) promptsView.findViewById(R.id.editText_rename_device);

                    //get current name
                    etUserInput.setText(gridAdapter.getDeviceIconAt((int) idx).getDeviceDescription());

                    dialogBuilder.setCancelable(true);
                    dialogBuilder.setPositiveButton("OK",
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int id) {
                                    newDescription[0] = etUserInput.getText().toString(); //what a smart way to modify "final" :)
                                    Log.v(c.tag_my_device_fragment, "rename device: GridView id=" + idx + " new name: " + newDescription[0]);

                                    //up date the device description: update device list first, then UI
                                    String mac = gridAdapter.getDeviceIconAt((int) idx).getDeviceMac();
                                    try {
                                        mDeviceList.setDeviceDescription(mac, newDescription[0]);
                                    } catch (JSONException e) {
                                        Log.e(c.tag_my_device_fragment, "gridAdapter->change device description JSONException e " + e);
                                    } catch (IOException e) {
                                        Log.e(c.tag_my_device_fragment, "gridAdapter->change device description  IOException e " + e);
                                    }

                                    gridAdapter.getDeviceIconAt((int) idx).setDeviceDescription(newDescription[0]);

                                    Toast toast = Toast.makeText(getActivity().getApplicationContext(), "New name: " + newDescription[0], Toast.LENGTH_SHORT);
                                    toast.show();

                                }
                            });
                    dialogBuilder.setNegativeButton("Cancel",
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int id) {
                                    dialog.cancel();
                                }
                            });

                    AlertDialog alertDialog = dialogBuilder.create();
                    alertDialog.show();

                } else {
                    //remove device: remove it form list first, then UI
                    final String mac = gridAdapter.getDeviceIconAt((int) idx).getDeviceMac();
                    new Thread(new Runnable() {
                        @Override
                        public void run() {
                            try {
                                Log.v("unpair-result:","before call \n");
                                final int result = unpairing(mac);
                                Log.v("unpair-result:","after call result ="+result+"\n");
                                final String deviceDescription = gridAdapter.getDeviceIconAt((int) idx).getDeviceDescription();
                                if(result==c.val_unpairing_succeed){
                                    getActivity().runOnUiThread(new Runnable() {
                                        @Override
                                        public void run() {
                                            try {
                                                gridAdapter.remove(gridAdapter.getDeviceIconAt((int) idx));
                                                mDeviceList.removeDevice(mac);
                                                Toast.makeText(getActivity().getApplicationContext(), deviceDescription + " removed", Toast.LENGTH_SHORT).show();
                                            } catch (Exception e) {
                                                Log.e(c.tag_my_device_fragment, "gridAdapter->remove device Exception e " + e);
                                            }
                                        }
                                    });
                                }
                                else {
                                    getActivity().runOnUiThread(new Runnable() {
                                        @Override
                                        public void run() {
                                            Toast.makeText(getActivity().getApplicationContext(), "Failed to unpair "+deviceDescription + ", Error code: "+result, Toast.LENGTH_LONG).show();
                                            //////////force remove prompt//////////
                                            AlertDialog.Builder dialogBuilder;
                                            dialogBuilder = new AlertDialog.Builder(getActivity());
                                            dialogBuilder.setTitle("Force to Remove Device ?");
                                            dialogBuilder.setCancelable(true);
                                            dialogBuilder.setPositiveButton("OK",
                                                    new DialogInterface.OnClickListener() {
                                                        public void onClick(DialogInterface dialog, int id) {

                                                            try {
                                                                gridAdapter.remove(gridAdapter.getDeviceIconAt((int) idx));
                                                                mDeviceList.removeDevice(mac);
                                                                Toast.makeText(getActivity().getApplicationContext(), deviceDescription + " removed", Toast.LENGTH_SHORT).show();
                                                            } catch (Exception e) {
                                                                Log.e(c.tag_my_device_fragment, "gridAdapter->Force to Remove device Exception e " + e);
                                                            }

                                                        }
                                                    });
                                            dialogBuilder.setNegativeButton("Cancel",
                                                    new DialogInterface.OnClickListener() {
                                                        public void onClick(DialogInterface dialog, int id) {
                                                            dialog.cancel();
                                                        }
                                                    });

                                            AlertDialog alertDialog = dialogBuilder.create();
                                            alertDialog.show();
                                            //////////force remove prompt//////////

                                        }
                                    });
                                }
                            }
                            catch (Exception e){}
                        }
                    }).start();
                }
            }
        });
        builder.show();
    }

    private synchronized int unpairing(String mac)throws Exception{
        if(mDeviceList.trackingListDeviceLookup(mac)==c.val_device_not_found)
        {
            return c.err_unpairing_device_not_found;
        }
        else {
            String ip = mDeviceList.trackingListGetDeviceIp(mac);
            connectionChecker = new ConnectionChecker();
            boolean deviceIsOnline = connectionChecker.testConnection(ip, c.val_ping_base_time_out,c.val_ping_max_try);
            if(!deviceIsOnline){
                mDeviceList.downListAddDevice(mac);
                return c.err_unpairing_server_ping_timeout;
            }
            else {
                int port = mDeviceList.trackingListGetDevicePort(mac);
                String key = mDeviceList.getDeviceSharedKey(mac);
                String txEnc = mAES.encryptToStringOfShortArray(c.tcp_tx_cmd_unpairing, key);
                TcpClient tcpClient = new TcpClient();
                String rx = tcpClient.echo(InetAddress.getByName(ip),port,txEnc);
                if (!rx.equals(c.tcp_rx_unpairing_ok)){
                    return c.err_unpairing_with_server_failed;
                }
                return c.val_unpairing_succeed;
            }
        }

    }

    private synchronized void startAmebaActivity(final String mac) throws JSONException{
        connectionChecker = new ConnectionChecker();
        String deviceDescription = mDeviceList.getDeviceDescription(mac);
        String controlType = mDeviceList.getDeviceControlType(mac);
        final String firebaseAppId = mDeviceList.getDeviceFirebaseAppId(mac);
        String key = mDeviceList.getDeviceSharedKey(mac);
        final String ip;
        final int port;
        final Intent intent;

        //test if ip and port is available, if not available, assign proper value to ip and port
        if(mDeviceList.trackingListDeviceLookup(mac)==c.val_device_not_found){
            ip = c.val_empty;
            port = c.val_device_not_found;
        }
        else {
            ip = mDeviceList.trackingListGetDeviceIp(mac);
            port = mDeviceList.trackingListGetDevicePort(mac);
        }

        //for ht sensor
        //if(serviceName.equals(c.nsd_service_ht)){
        intent = new Intent(getActivity(), HTSensorActivity.class);
        intent.putExtra(c.key_ip,ip);
        intent.putExtra(c.key_port,port);
        intent.putExtra(c.key_mac,mac);
        intent.putExtra(c.key_control_type,controlType);
        intent.putExtra(c.key_firebase_app_id,firebaseAppId);
        intent.putExtra(c.key_shared_key, key);
        intent.putExtra(c.key_device_description,deviceDescription);
        //}

        /*//for XXX sensor:
        else if (serviceName.equals(c.nsd_service_XXX)){
            ....
        }*/


        new Thread(new Runnable() {
            @Override
            public void run() {
                ////////////////////////////////toast on ui////////////////////////////////
                getActivity().runOnUiThread(new Runnable() {
                    public void run() {
                        Toast.makeText(getActivity(), "Checking device status...", Toast.LENGTH_LONG).show();
                    }
                });
                ////////////////////////////////toast on ui////////////////////////////////

                //check if the device is online
                boolean deviceIsOnline;
                String controlType=c.val_empty;
                try {
                    controlType=mDeviceList.getDeviceControlType(mac);
                }catch (JSONException e){Log.e(c.tag_my_device_fragment,"startAmebaActivity() JSONException e"+e);}

                if(controlType.equals(c.val_local_control)){
                    if(ip.equals(c.val_empty)){
                        deviceIsOnline=false;
                        Log.v(c.tag_my_device_fragment,"startAmebaActivity() ping local device "+mac+" with ip "+ip+" -- device is not found in tracking list");
                    }
                    else {
                        Log.v(c.tag_my_device_fragment,"startAmebaActivity() ping local device "+mac+" with ip "+ip);
                        deviceIsOnline= connectionChecker.testConnection(ip, c.val_ping_base_time_out,c.val_ping_max_try);
                    }
                }
                else {
                    //cloud device, if its in tracking list, it must be validate by local ping
                    if(!ip.equals(c.val_empty)){
                        Log.v(c.tag_my_device_fragment,"startAmebaActivity() ping cloud device "+mac+" locally with ip "+ip);
                        deviceIsOnline= connectionChecker.testConnection(ip, c.val_ping_base_time_out,c.val_ping_max_try);
                    }
                    else {
                        Log.v(c.tag_my_device_fragment,"startAmebaActivity() check cloud device "+mac+" with firebase app Id "+firebaseAppId);
                        deviceIsOnline=connectionChecker.testConnection(firebaseAppId);
                    }
                }


                if(deviceIsOnline){
                    mDeviceList.downListRemoveDevice(mac);
                    ////////////////////////////////toast on ui////////////////////////////////
                    getActivity().runOnUiThread(new Runnable() {
                        public void run() {
                            Toast.makeText(getActivity(), "Connect to Ameba...", Toast.LENGTH_SHORT).show();
                        }
                    });
                    ////////////////////////////////toast on ui////////////////////////////////
                    startActivity(intent);
                }
                else {
                    mDeviceList.downListAddDevice(mac);
                    ////////////////////////////////toast on ui////////////////////////////////
                    getActivity().runOnUiThread(new Runnable() {
                        public void run() {
                            Toast.makeText(getActivity(), "Device is offline", Toast.LENGTH_LONG).show();
                        }
                    });
                    ////////////////////////////////toast on ui////////////////////////////////
                }
            }
        }).start();
    }

    /**
     * Prepare data for gridview
     */
    private synchronized ArrayList<DeviceIcon> getDeviceIcons() throws JSONException{
        final ArrayList<DeviceIcon> deviceIcons = new ArrayList<>();
        for (int i = 0; i < mDeviceList.size(); i++) {
            String mac = mDeviceList.getDeviceMac(i);
            String serviceName = mDeviceList.getDeviceServiceName(mac);
            String controlType = mDeviceList.getDeviceControlType(mac);
            Bitmap icon;

            /////////////////////////////icon/////////////////////////////
            if(controlType.equals(c.val_local_control)){
                if(mDeviceList.trackingListDeviceLookup(mac)==c.val_device_not_found){
                    icon = mDeviceList.getDeviceIcon(serviceName,controlType,false);
                }
                else if(mDeviceList.downListLookup(mac)!=c.val_device_not_found){
                    icon = mDeviceList.getDeviceIcon(serviceName,controlType,false);
                }
                else {
                    icon = mDeviceList.getDeviceIcon(serviceName,controlType,true);
                }
            }
            else {
                if(mDeviceList.downListLookup(mac)!=c.val_device_not_found){
                    icon = mDeviceList.getDeviceIcon(serviceName,controlType,false);
                }
                else {
                    icon = mDeviceList.getDeviceIcon(serviceName,controlType,true);
                }
            }
            /////////////////////////////icon/////////////////////////////

            String description = mDeviceList.getDeviceDescription(mac);

            deviceIcons.add(new DeviceIcon(icon,description,mac));
        }
        return deviceIcons;
    }


    ///////////////////////////update ui////////////////////////////

    Runnable updateUiCallback = new Runnable() {
        @Override
        public void run() {
            ////////////////////update ui////////////////////
            if(getActivity() != null) {
                try {
                    gridAdapter = new GridViewAdapter(getActivity(), R.layout.layout_grid_view_item, getDeviceIcons());
                    gridView.setAdapter(gridAdapter);
                } catch (JSONException e) {
                    Log.e(c.tag_my_device_fragment, "updateUiCallback JSONException e " + e);
                }
            }
            ////////////////////update ui////////////////////
            mHandler.postDelayed(updateUiCallback, c.val_ui_update_period_ms);
        }
    };

    void startUpdateUi() {
        updateUiCallback.run();
    }

    void stopUpdateUi() {
        mHandler.removeCallbacks(updateUiCallback);
    }

    ///////////////////////////update ui////////////////////////////

}

