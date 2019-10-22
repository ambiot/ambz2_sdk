package com.realtek.wigadget;

import android.util.Log;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.InetAddress;
import java.net.MalformedURLException;
import java.net.URL;

/**
 * Created by Wu Jinzhou on 5/15/2015.
 */
public class ConnectionChecker {

    private static final Constants c = Constants.getInstance();

    public synchronized boolean testConnection(String ip, int baseTimeOut, int maxTry){
        try {
            InetAddress serverAddr = InetAddress.getByName(ip);
            for(int i = 1; i< maxTry+1 ; i++){
                Log.v(c.tag_connection_checker,"testConnection() checking ip "+ip+" with timeout "+(baseTimeOut*i));
                if(serverAddr.isReachable(baseTimeOut*i)){
                    return true;
                }
            }
        }
        catch (IOException e){Log.e(c.tag_connection_checker,"testConnection() IOException e "+e);}
        return false;
    }

    public synchronized boolean testConnection(String firebaseAppId){

        int responseCode = 404;
        String firebaseURL;

        if(firebaseAppId.endsWith(".firebaseio.com")){
            firebaseURL = "https://"+firebaseAppId;
        }
        else{
            firebaseURL = "https://"+firebaseAppId+".firebaseio.com";
        }

        try{
            Log.v(c.tag_connection_checker,"testConnection() checking FirebaseAppId "+firebaseAppId);
            URL url = new URL(firebaseURL);
            HttpURLConnection connection = (HttpURLConnection)url.openConnection();
            connection.setRequestMethod("GET");
            connection.connect();
            responseCode = connection.getResponseCode();
        }
        catch (MalformedURLException e){
            Log.e(c.tag_connection_checker,"testConnection() for cloud -- MalformedURLException e"+e);
        }
        catch (IOException e){
            Log.e(c.tag_connection_checker,"testConnection() for cloud -- IOException e"+e);
        }

        if(responseCode == 200){
            return true;
        }
        return false;
    }
}
