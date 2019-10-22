package com.realtek.wigadget;

import android.app.AlertDialog;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.drawable.AnimationDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.EditText;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.Toast;

import org.json.JSONException;

import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;

import javax.jmdns.ServiceEvent;

public class FindDeviceFragment extends Fragment {
    static final Constants c = Constants.getInstance();
    private AES128 mAES = new AES128();
    private GridView gridView;
    private GridViewAdapter gridAdapter;
    private DeviceList mDeviceList;
    private ConnectionChecker connectionChecker;

    private Handler mHandler;

    public FindDeviceFragment() {
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getActivity().setTitle(R.string.find_device_fragment_title);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

        View rootView = inflater.inflate(R.layout.fragment_find_device, container, false);

        try {
            mDeviceList = DeviceList.getInstance(getActivity().getApplicationContext());
        } catch (JSONException e) {
            Log.e(c.tag_find_device_fragment, "init mDeviceList JSONException e --" + e);
        } catch (IOException e) {
            Log.e(c.tag_find_device_fragment, "init mDeviceList IOException e --" + e);
        }

        ///////////////////////////////////show animation///////////////////////////////////
        ImageView iv = (ImageView) rootView.findViewById(R.id.animation);
        iv.setBackgroundResource(R.drawable.animation);
        final AnimationDrawable anim = (AnimationDrawable) iv.getBackground();

        rootView.post(new Runnable() {
            @Override
            public void run() {
                anim.start();
            }
        });
        ///////////////////////////////////show animation///////////////////////////////////


        /////////////////////////////////set gridview/////////////////////////////////////////

        gridView = (GridView) rootView.findViewById(R.id.find_device_gridView);
        try {
            gridAdapter = new GridViewAdapter(getActivity(), R.layout.layout_grid_view_item, getDeviceIcons());
        } catch (JSONException e) {
            Log.e(c.tag_find_device_fragment, "JSONException e --" + e);
        }

        gridView.setAdapter(gridAdapter);
        gridView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View v, int position, long id) {

                //get mac addr from position:
                String mac = gridAdapter.getDeviceIconAt(position).getDeviceMac();
                Log.v(c.tag_find_device_fragment, "clicked on device :" + mac);
                //pair device
                popupOptions(id);
            }
        });
        /////////////////////////////////set gridview/////////////////////////////////////////


        /////////////////////////////////update ui thread/////////////////////////////////////
        mHandler=new Handler();
        startUpdateUi();
        /////////////////////////////////update ui thread/////////////////////////////////////


        return rootView;
    }


    private void popupOptions(final long id) {

        final boolean alreadyPaired;

        //test if the device is paired
        String mMAC = gridAdapter.getDeviceIconAt((int) id).getDeviceMac();
        ServiceEvent serviceEvent = mDeviceList.tempListGetDeviceServiceEvent(mMAC);
        if (serviceEvent.getInfo().getPropertyString(c.key_pair_state).equals(c.val_paired)) {
            alreadyPaired = true;
        } else {
            alreadyPaired = false;
        }

        //setup popup
        final ImageListAdapter adapter;
        if (alreadyPaired) {
            adapter = new ImageListAdapter(getView().getContext(), getResources().getStringArray(R.array.find_device_popup_share));
        } else {
            adapter = new ImageListAdapter(getView().getContext(), getResources().getStringArray(R.array.find_device_popup_pair));
        }

        final AlertDialog.Builder builder = new AlertDialog.Builder(getView().getContext());


        builder.setTitle(R.string.popup_dialog_title_options);
        builder.setCancelable(true);

        builder.setAdapter(adapter, new DialogInterface.OnClickListener() {

            @Override
            public void onClick(DialogInterface dialog, int index) {

                ///////////////switch controls/////////////////
                if (index == 0) {
                    if (alreadyPaired) {
                        /*share device*/
                        Toast toast = Toast.makeText(getActivity().getApplicationContext(), "Device Sharing\nWill be supported in the future release", Toast.LENGTH_SHORT);
                        toast.show();
                    } else {
                        final String mac = gridAdapter.getDeviceIconAt((int) id).getDeviceMac();
                        final String controlType = mDeviceList.tempListGetDeviceServiceEvent(mac).getInfo().getPropertyString(c.key_control_type);
                        final String[] firebaseAppId = {c.val_empty};

                        final String description = gridAdapter.getDeviceIconAt((int) id).getDeviceDescription();
                        final int[] result = {c.err_pairing_failed_to_start};

                        ////////////////////////////////////////pairing blocks////////////////////////////////////////
                        if (controlType.equals(c.val_cloud_control)) {
                            //cloud pairing process: 1. get firebase app id 2. pairing
                            LayoutInflater inflater;
                            View promptsView;
                            AlertDialog.Builder dialogBuilder;
                            final EditText etUserInput;

                            inflater = LayoutInflater.from(getActivity());
                            promptsView = inflater.inflate(R.layout.layout_popup_dialog_prompts_firebase_app_id, null);

                            dialogBuilder = new AlertDialog.Builder(getActivity());
                            dialogBuilder.setView(promptsView);
                            etUserInput = (EditText) promptsView.findViewById(R.id.editText_firebase_app_id);

                            dialogBuilder.setCancelable(true);

                            dialogBuilder.setPositiveButton("OK",
                                    new DialogInterface.OnClickListener() {
                                        public void onClick(DialogInterface dialog, int id) {
                                            firebaseAppId[0] = etUserInput.getText().toString();
                                            if (firebaseAppId[0].equals(c.val_empty)) {
                                                Toast.makeText(getActivity().getApplicationContext(), "Firebase app id is empty, pairing canceled", Toast.LENGTH_LONG).show();
                                            } else {
                                                /////////////////////////////cloud pairing///////////////////////////////
                                                final ProgressDialog progress = ProgressDialog.show(getActivity(), "Device Pairing", "Adding new device: " + description, true);
                                                new Thread(new Runnable() {
                                                    @Override
                                                    public void run() {
                                                        connectionChecker = new ConnectionChecker();

                                                        getActivity().runOnUiThread(new Runnable() {
                                                            public void run() {
                                                                Toast.makeText(getActivity().getApplicationContext(), "Verifying Firebase App Id: " + firebaseAppId[0], Toast.LENGTH_SHORT).show();
                                                            }
                                                        });

                                                        if (connectionChecker.testConnection(firebaseAppId[0])) {
                                                            result[0] = pairing(mac, firebaseAppId[0]);
                                                        }
                                                        else {

                                                            getActivity().runOnUiThread(new Runnable() {
                                                                public void run() {
                                                                    Toast.makeText(getActivity().getApplicationContext(), "Verify Firebase App Id Failed", Toast.LENGTH_SHORT).show();
                                                                }
                                                            });

                                                            result[0] = c.err_firebase_app_url_cannot_be_verified;
                                                        }

                                                        getActivity().runOnUiThread(new Runnable() {
                                                            @Override
                                                            public void run() {
                                                                progress.dismiss();
                                                                if (result[0] == c.val_pairing_succeed) {
                                                                    Toast.makeText(getActivity().getApplicationContext(), "Pairing succeed, device saved !", Toast.LENGTH_SHORT).show();
                                                                    ////////////////////////switch fragment////////////////////////
                                                                    Fragment fragment = new MyDeviceFragment();
                                                                    FragmentManager fragmentManager = getFragmentManager();
                                                                    fragmentManager.beginTransaction().replace(R.id.frame_container, fragment).commit();
                                                                    ////////////////////////switch fragment////////////////////////
                                                                } else {
                                                                    Toast.makeText(getActivity().getApplicationContext(), "Pairing failed, err code: " + result[0], Toast.LENGTH_LONG).show();
                                                                }
                                                            }
                                                        });
                                                    }
                                                }).start();
                                                ////////////////////////////cloud pairing////////////////////////////////
                                            }

                                        }
                                    });

                            dialogBuilder.setNeutralButton("Register Firebase",
                                    new DialogInterface.OnClickListener() {
                                        public void onClick(DialogInterface dialog, int id) {

                                            Intent intent = new Intent(getActivity(), RegisterFirebaseAccountActivity.class);
                                            startActivity(intent);

                                        }
                                    });

                            AlertDialog alertDialog = dialogBuilder.create();
                            alertDialog.show();

                        } else {
                            //local pairing process
                            /////////////////////////////local pairing///////////////////////////////
                            final ProgressDialog progress = ProgressDialog.show(getActivity(), "Device Pairing", "Adding new device: " + description, true);

                            new Thread(new Runnable() {
                                @Override
                                public void run() {

                                    result[0] = pairing(mac, firebaseAppId[0]);

                                    getActivity().runOnUiThread(new Runnable() {
                                        @Override
                                        public void run() {
                                            progress.dismiss();
                                            if (result[0] == c.val_pairing_succeed) {
                                                Toast.makeText(getActivity().getApplicationContext(), "Pairing succeed, device saved !", Toast.LENGTH_SHORT).show();
                                                ////////////////////////switch fragment////////////////////////
                                                Fragment fragment = new MyDeviceFragment();
                                                FragmentManager fragmentManager = getFragmentManager();
                                                fragmentManager.beginTransaction().replace(R.id.frame_container, fragment).commit();
                                                ////////////////////////switch fragment////////////////////////
                                            } else {
                                                Toast.makeText(getActivity().getApplicationContext(), "Pairing failed, err code: " + result[0], Toast.LENGTH_LONG).show();
                                            }
                                        }
                                    });
                                }
                            }).start();

                        }
                        /////////////////////////////local pairing///////////////////////////////

                        ////////////////////////////////////////pairing blocks////////////////////////////////////////

                    }
                } else {
                    //nothing, only one item in the list
                }
            }
        });
        builder.show();
    }


    /**
     * Prepare data for gridview
     */
    private synchronized ArrayList<DeviceIcon> getDeviceIcons() throws JSONException {
        final ArrayList<DeviceIcon> deviceIcons = new ArrayList<>();
        for (int i = 0; i < mDeviceList.tempListSize(); i++) {
            String mac = mDeviceList.tempListGetMac(i);

            String name = mDeviceList.tempListGetDeviceServiceEvent(mac).getInfo().getPropertyString(c.key_service_name);
            String controlType = mDeviceList.tempListGetDeviceServiceEvent(mac).getInfo().getPropertyString(c.key_control_type);
            String append = mac;
            if(mac.length()<5){
                append=mac+"WU:JZ";
            }
            if (mDeviceList.tempListGetDeviceServiceEvent(mac).getInfo().getPropertyString(c.key_pair_state).equals(c.val_paired)) {
                name += " "+append.substring((append.length()-5),(append.length()))+" (Paired)";
            } else {
                name += " "+append.substring((append.length()-5),(append.length()))+" (Unpaired)";
            }

            Bitmap icon;
            if (mDeviceList.tempListGetDeviceServiceEvent(mac).getInfo().getPropertyString(c.key_service_name).equals(c.nsd_service_ht)) {
                icon = mDeviceList.getDeviceIcon(c.nsd_service_ht, controlType, true);
            } else {
                icon = mDeviceList.getDeviceIcon(c.nsd_service_unknown, controlType, true);
            }

            deviceIcons.add(new DeviceIcon(icon, name, mac));
        }
        return deviceIcons;
    }


    private int pairing (String mac, String firebaseAppId){

        final String ip = mDeviceList.tempListGetDeviceServiceEvent(mac).getInfo().getHostAddresses()[0];
        final int port = mDeviceList.tempListGetDeviceServiceEvent(mac).getInfo().getPort();

        try {

            KeyGen keyGen = KeyGen.getInstance();

            TcpClient tcpClient = new TcpClient();

            String rx = tcpClient.echo(InetAddress.getByName(ip),port,c.tcp_tx_pair);

            if(rx==null){
                return c.err_server_no_response;
            }

            if(rx.equals(c.tcp_rx_error)){
                return c.err_pair_cmd_rejected;
            }

            keyGen.makeKeys(rx);

            rx = tcpClient.echo(InetAddress.getByName(ip), port, keyGen.getPublicKeyString());

            if(rx==null){
                return c.err_server_no_response;
            }

            if (rx.equals(c.tcp_rx_error)){
                return c.err_public_key_rejected;
            }

            String serviceName = mDeviceList.tempListGetDeviceServiceEvent(mac).getInfo().getPropertyString(c.key_service_name);
            String controlType = mDeviceList.tempListGetDeviceServiceEvent(mac).getInfo().getPropertyString(c.key_control_type);
            String aes128Key = keyGen.getAES128KeyString();

            String append =mac;
            if(mac.length()<5){
                append=mac+"WU:JZ";
            }

            String description = "[New] "+serviceName +" "+append.substring((append.length()-5),(append.length()));

            if(rx.equals(c.tcp_rx_pair_ok)){
                try {
                    //if device with same mac is already in the list, remove it first
                    int idx = mDeviceList.deviceLookup(mac);
                    if(idx!=c.val_device_not_found){
                        mDeviceList.removeDevice(mac);
                    }
                    mDeviceList.transientListAddDevice(mDeviceList.tempListGetDeviceServiceEvent(mac));
                    mDeviceList.addDevice(mac,serviceName,controlType,firebaseAppId,aes128Key,description);
                }
                catch (JSONException e){Log.e(c.tag_find_device_fragment,"pairing() JSONException e "+e);}

                return c.val_pairing_succeed;
            }

            if(rx.equals(c.tcp_rx_firebase_app_url)){
                String firebaseAppUrl = "https://"+firebaseAppId+".firebaseio.com";
                String txEnc=null;

                try {
                    txEnc= mAES.encryptToStringOfShortArray(firebaseAppUrl,aes128Key);
                }
                catch (Exception e){Log.e(c.tag_find_device_fragment,"pairing() Exception e"+e);}

                rx = tcpClient.echo(InetAddress.getByName(ip), port, txEnc);

                if(rx==null){
                    return c.err_server_no_response;
                }

                if(rx.equals(c.tcp_rx_error)){
                    return c.err_firebase_app_url_rejected;
                }

                if(rx.equals(c.tcp_rx_pair_ok)){
                    try {
                        //if device with same mac is already in the list, remove it first
                        int idx = mDeviceList.deviceLookup(mac);
                        if(idx!=c.val_device_not_found){
                            mDeviceList.removeDevice(mac);
                        }
                        mDeviceList.transientListAddDevice(mDeviceList.tempListGetDeviceServiceEvent(mac));
                        mDeviceList.addDevice(mac,serviceName,controlType,firebaseAppId,aes128Key,description);
                    }
                    catch (JSONException e){Log.e(c.tag_find_device_fragment,"pairing() JSONException e "+e);}
                    return c.val_pairing_succeed;
                }
            }
            return c.err_unknown_error;
        }
        catch (UnknownHostException e){
            Log.e(c.tag_find_device_fragment, "pairing() UnknownHostException e " + e);
            return c.err_unknown_host_exception;
        }
        catch (InterruptedException e){
            Log.e(c.tag_find_device_fragment,"InterruptedException e"+e);
            return c.err_interrupted_exception;
        }
        catch (IOException e){
            Log.e(c.tag_find_device_fragment,"IOException e"+e);
            return c.err_io_exception;
        }
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
                    Log.e(c.tag_find_device_fragment, "updateUiCallback JSONException e " + e);
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

