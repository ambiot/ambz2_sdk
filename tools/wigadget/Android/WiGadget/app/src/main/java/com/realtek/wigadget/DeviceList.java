package com.realtek.wigadget;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.Log;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Vector;

import javax.jmdns.ServiceEvent;

/**
 * Created by Wu Jinzhou on 4/15/2015.
 */
public class DeviceList {
    static final Constants c =Constants.getInstance();

    private final String tag = c.tag_device_list;

    private static DeviceList thisDeviceList;
    private static Vector<ServiceEvent> tempList = new Vector<>();
    private static Vector<ServiceEvent> trackingList = new Vector<>();
    private static Vector<ServiceEvent> transientList = new Vector<>();
    private static Vector<String> downList = new Vector<>();
    private static JSONArray jArray;

    private static Context mContext;

    private DeviceList(Context ctx){
        mContext=ctx;
    }

    public synchronized boolean addDevice (String mac, String serviceName, String controlType, String firebaseAppId, String sharedKey, String description) throws JSONException,IOException {
        if (deviceLookup(mac)==c.val_device_not_found){

            JSONObject jObject =new JSONObject();
            String currentDateTime = new SimpleDateFormat(c.key_date_format).format(new Date());

            jObject.put(c.key_mac,mac);
            jObject.put(c.key_service_name,serviceName);
            jObject.put(c.key_control_type,controlType);
            jObject.put(c.key_firebase_app_id,firebaseAppId);
            jObject.put(c.key_shared_key,sharedKey);
            jObject.put(c.key_device_description,description);
            jObject.put(c.key_date,currentDateTime);
            jArray.put(jObject);

            //save change to file
            saveChange();
            Log.v(tag,"added device :"+mac);
            return true;
        }
        else {
            Log.v(tag,"device "+mac+" already in list");
            return false;
        }

    }

    public synchronized boolean removeDevice(String mac)throws JSONException,IOException{

        int idxToBeRemoved = deviceLookup(mac);

        if(idxToBeRemoved!=c.val_device_not_found){

            Vector jVector = new Vector<JSONObject>();

            for (int i=0;i<jArray.length();i++){
                if(i!=idxToBeRemoved){
                    jVector.add(jArray.getJSONObject(i));
                }
            }

            jArray = new JSONArray();

            for (int i=0;i<jVector.size();i++){
                jArray.put(jVector.elementAt(i));
            }

            //also remove device it its in trackingList, transientList and downList
            if(trackingListDeviceLookup(mac)!=c.val_device_not_found){
                trackingListRemoveDevice(mac);
                Log.v(tag,"trackingListRemoveDevice() "+mac);
            }

            if(transientListLookup(mac)!=c.val_device_not_found){
                transientListRemoveDevice(mac);
                Log.v(tag, "transientListRemoveDevice() " + mac);
            }

            if(downListLookup(mac)!=c.val_device_not_found){
                downListRemoveDevice(mac);
                Log.v(tag, "downListRemoveDevice() " + mac);
            }

            //save change to file
            saveChange();
            Log.v(tag,"removed device :"+mac);
            return true;
        }
        else {
            Log.v(tag,"device "+mac+" not found");
            return false;
        }
    }


    public synchronized int deviceLookup(String mac) throws JSONException{
        for (int i=0;i<jArray.length();i++)
        {
            if (jArray.getJSONObject(i).getString(c.key_mac).equals(mac)){
                return i;
            }
        }
        return c.val_device_not_found;
    }

    private static synchronized void load() throws JSONException,IOException{

        String filePath = mContext.getFilesDir().getPath()+"/"+c.val_file_name;
        File f = new File(filePath);

        if (!f.exists()){
            f.createNewFile();
        }

        FileInputStream fis=mContext.openFileInput(c.val_file_name);
        BufferedInputStream bis=new BufferedInputStream(fis);
        StringBuffer sb=new StringBuffer();
        while (bis.available()!=0){
            char c=(char)bis.read();
            sb.append(c);
        }
        bis.close();
        fis.close();

        if(sb.toString().equals("")){
            jArray=new JSONArray();
        }
        else {
            jArray=new JSONArray(sb.toString());
        }

    }

    private synchronized void saveChange() throws IOException,JSONException{
        FileOutputStream fos;
        fos = mContext.openFileOutput(c.val_file_name, Context.MODE_PRIVATE);
        fos.write(jArray.toString(4).getBytes());
        fos.close();
    }

    public synchronized Bitmap getDeviceIcon(String serviceName, String controlType, boolean online)throws JSONException{

        TypedArray imgArray;
        Bitmap bitmap;

        if(serviceName.equals(c.nsd_service_ht)){
            imgArray=mContext.getResources().obtainTypedArray(R.array.device_ht);
            if((online)&&(controlType.equals(c.val_local_control))) {
                //local online
                bitmap = BitmapFactory.decodeResource(mContext.getResources(), imgArray.getResourceId(0, -1));
            }
            else if((!online)&&(controlType.equals(c.val_local_control))) {
                //local offline
                bitmap = BitmapFactory.decodeResource(mContext.getResources(), imgArray.getResourceId(1, -1));
            }
            else if((online)&&(controlType.equals(c.val_cloud_control))){
                //cloud online
                bitmap = BitmapFactory.decodeResource(mContext.getResources(), imgArray.getResourceId(2, -1));
            }
            else {
                //cloud offline
                bitmap = BitmapFactory.decodeResource(mContext.getResources(), imgArray.getResourceId(3, -1));
            }
        }

        //else if(serviceName.equals(xxxx_xxxxx)....)
        // {..
        //
        // ..}

        else{
            imgArray=mContext.getResources().obtainTypedArray(R.array.device_unknown);

            if((online)&&(controlType.equals(c.val_local_control))) {
                //local online
                bitmap = BitmapFactory.decodeResource(mContext.getResources(), imgArray.getResourceId(0, -1));
            }
            else if((!online)&&(controlType.equals(c.val_local_control))) {
                //local offline
                bitmap = BitmapFactory.decodeResource(mContext.getResources(), imgArray.getResourceId(1, -1));
            }
            else if((online)&&(controlType.equals(c.val_cloud_control))){
                //cloud online
                bitmap = BitmapFactory.decodeResource(mContext.getResources(), imgArray.getResourceId(2, -1));
            }
            else {
                //cloud offline
                bitmap = BitmapFactory.decodeResource(mContext.getResources(), imgArray.getResourceId(3, -1));
            }
        }
        imgArray.recycle();
        return bitmap;
    }

    public synchronized String getDeviceMac(int idx)throws JSONException{
        if((idx<0)||(idx>jArray.length())){
            return null;
        }
        return jArray.getJSONObject(idx).getString(c.key_mac);
    }

    public synchronized String getDeviceServiceName(String mac)throws JSONException{
        int idx=deviceLookup(mac);
        if(idx==c.val_device_not_found){
            return null;
        }
        return jArray.getJSONObject(idx).getString(c.key_service_name);
    }


    public synchronized String getDeviceControlType(String mac)throws JSONException{
        int idx=deviceLookup(mac);
        if (idx==c.val_device_not_found){
            return null;
        }
        return jArray.getJSONObject(idx).getString(c.key_control_type);
    }


    public synchronized String getDeviceSharedKey(String mac) throws JSONException{
        int idx=deviceLookup(mac);
        if(idx==c.val_device_not_found){
            return null;
        }
        return jArray.getJSONObject(idx).getString(c.key_shared_key);
    }

    public synchronized String getDeviceDescription(String mac)throws JSONException{
        int idx=deviceLookup(mac);
        if(idx==c.val_device_not_found){
            return null;
        }
        else {
            return jArray.getJSONObject(idx).getString(c.key_device_description);
        }
    }

    public synchronized boolean setDeviceDescription(String mac, String description)throws JSONException,IOException{
        int idx=deviceLookup(mac);
        if(idx==c.val_device_not_found){
            return false;
        }
        else {
            jArray.getJSONObject(idx).put(c.key_device_description,description);
            saveChange();
            return true;
        }
    }

    public synchronized String getDeviceFirebaseAppId(String mac)throws JSONException{
        int idx =deviceLookup(mac);
        if(idx==c.val_device_not_found){
            return null;
        }
        else if(jArray.getJSONObject(idx).getString(c.key_control_type).equals(c.val_local_control)){
            return null;
        }
        else {
            return jArray.getJSONObject(idx).getString(c.key_firebase_app_id);
        }
    }
    public synchronized int size(){
        return jArray.length();
    }

    public synchronized String tempListGetMac(int idx){
        if((idx<0)||(idx>tempList.size())){
            return null;
        }
        return tempList.elementAt(idx).getInfo().getPropertyString(c.key_mac);
    }

    public synchronized ServiceEvent tempListGetDeviceServiceEvent(String mac){
        for (int i=0;i<tempList.size();i++){
            if (mac.equals(tempListGetMac(i))) {
                return tempList.elementAt(i);
            }
        }
        return null;
    }

    public synchronized void tempListUpdate(ServiceEvent serviceEvent)throws JSONException,IOException{
        String mac = serviceEvent.getInfo().getPropertyString(c.key_mac);
        String pairState = serviceEvent.getInfo().getPropertyString(c.key_pair_state);
        if((pairState.equals(c.val_not_paired))&&(tempListDeviceLookup(mac)==c.val_device_not_found)){
            tempList.add(serviceEvent);
            Log.v(c.tag_device_list,"tempListUpdate(): tempList.add("+mac+")"+"with pair state "+pairState);
            if((deviceLookup(mac)!=c.val_device_not_found)&&(transientListLookup(mac)==c.val_device_not_found)){
                removeDevice(mac);
                Log.v(c.tag_device_list, "tempListUpdate(): removeDevice(" + mac + ")"+"with pair state "+pairState);
            }
        }
        else {
            if(transientListLookup(mac)!=c.val_device_not_found){
                tempListRemoveDevice(mac);
                Log.v(c.tag_device_list, "tempListUpdate(): tempListRemoveDevice(" + mac + ")"+"with pair state "+pairState);
                transientListRemoveDevice(mac);
                Log.v(c.tag_device_list, "tempListUpdate(): transientListRemoveDevice(" + mac + ")"+"with pair state "+pairState);
            }
            else {
                if((tempListDeviceLookup(mac)==c.val_device_not_found)&&(deviceLookup(mac)==c.val_device_not_found)){
                    tempList.add(serviceEvent);
                    Log.v(c.tag_device_list, "tempListUpdate(): tempList.add(" + mac + ")"+"with pair state "+pairState);
                }
            }
            trackingListUpdate(serviceEvent);
        }
    }

    public synchronized boolean tempListRemoveDevice(String mac){
        int idx=tempListDeviceLookup(mac);
        if(idx==c.val_device_not_found){
            return false;
        }
        tempList.remove(idx);
        return true;
    }


    public synchronized int tempListDeviceLookup(String mac){
        for (int i=0;i<tempList.size();i++)
        {
            if (mac.equals(tempListGetMac(i))){
                return i;
            }
        }
        return c.val_device_not_found;
    }

    public synchronized int tempListSize(){
        return tempList.size();
    }

    public synchronized String trackingListGetMac(int idx){
        if((idx<0)||(idx>trackingList.size())){
            return null;
        }
        return trackingList.elementAt(idx).getInfo().getPropertyString(c.key_mac);
    }

    public synchronized int trackingListDeviceLookup(String mac){
        for (int i=0;i<trackingList.size();i++)
        {
            if (mac.equals(trackingListGetMac(i))){
                return i;
            }
        }
        return c.val_device_not_found;
    }

    public synchronized boolean trackingListFindDeviceOnline(String mac){
        int idx = trackingListDeviceLookup(mac);

        if(idx==c.val_device_not_found){
            return false;
        }

        ServiceEvent event = trackingListGetDeviceServiceEvent(mac);
        String mPairState = event.getInfo().getPropertyString(c.key_pair_state);

        if(mPairState.equals(c.val_not_paired)){
            return false;
        }

        return true;
    }

    public synchronized String trackingListGetDeviceIp(String mac){
        int idx = trackingListDeviceLookup(mac);
        if(idx!=c.val_device_not_found){
            return trackingListGetDeviceServiceEvent(mac).getInfo().getPropertyString(c.key_ip);
        }
        return null;
    }

    public synchronized int trackingListGetDevicePort(String mac){
        int idx = trackingListDeviceLookup(mac);
        if(idx!=c.val_device_not_found){
            return Integer.parseInt(trackingListGetDeviceServiceEvent(mac).getInfo().getPropertyString(c.key_port));
        }
        return c.val_device_not_found;
    }

    public synchronized ServiceEvent trackingListGetDeviceServiceEvent(String mac){
        for (int i=0;i<trackingList.size();i++){
            if (mac.equals(trackingListGetMac(i))) {
                return trackingList.elementAt(i);
            }
        }
        return null;
    }


    public synchronized void trackingListUpdate(ServiceEvent serviceEvent)throws JSONException{
        String mac = serviceEvent.getInfo().getPropertyString(c.key_mac);
        String pairState = serviceEvent.getInfo().getPropertyString(c.key_pair_state);
        if(trackingListDeviceLookup(mac)!=c.val_device_not_found){
            trackingListRemoveDevice(mac);
            Log.v(c.tag_device_list, "trackingListUpdate(): trackingListRemoveDevice(" + mac + ") with pair state " + pairState);
        }
        trackingList.add(serviceEvent);
        downListRemoveDevice(mac);
        Log.v(c.tag_device_list, "trackingListUpdate(): trackingList.add(" + mac + ") with pair state "+pairState);
    }

    public synchronized boolean trackingListRemoveDevice(String mac){
        int idx = trackingListDeviceLookup(mac);
        if(idx!=c.val_device_not_found){
            trackingList.remove(idx);
            return true;
        }
        return false;
    }

    public synchronized int transientListLookup(String mac){
        for(int i= 0; i<transientList.size();i++){
            if(mac.equals(transientList.get(i).getInfo().getPropertyString(c.key_mac))){
                return i;
            }
        }
        return c.val_device_not_found;
    }

    public synchronized boolean transientListRemoveDevice(String mac){
        int idx = transientListLookup(mac);
        if(idx!=c.val_device_not_found){
            transientList.remove(idx);
            return true;
        }
        return false;
    }

    public synchronized boolean transientListAddDevice(ServiceEvent serviceEvent){
        String mac = serviceEvent.getInfo().getPropertyString(c.key_mac);
        int idx = transientListLookup(mac);
        if (idx ==c.val_device_not_found){
            transientList.add(serviceEvent);
            return true;
        }
        return false;
    }

    public synchronized int downListLookup(String mac){
        for (int i=0;i<downList.size();i++){
            if(downList.elementAt(i).equals(mac)){
                return i;
            }
        }
        return c.val_device_not_found;
    }

    public synchronized boolean downListRemoveDevice(String mac){
        int idx=downListLookup(mac);
        if(idx!=c.val_device_not_found){
            downList.remove(idx);
            return true;
        }
        return false;
    }

    public synchronized boolean downListAddDevice(String mac){
        int idx=downListLookup(mac);
        if(idx==c.val_device_not_found){
            downList.addElement(mac);
            return true;
        }
        return false;
    }

    public static synchronized DeviceList getInstance(Context mContext)throws JSONException,IOException{
        if(thisDeviceList ==null){
            thisDeviceList = new DeviceList(mContext);
            load();
        }
        return thisDeviceList;
    }

}
