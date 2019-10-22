package com.realtek.wigadget;

import android.app.Activity;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.widget.TextView;

import com.androidplot.ui.SizeLayoutType;
import com.androidplot.ui.SizeMetrics;
import com.androidplot.xy.BoundaryMode;
import com.androidplot.xy.LineAndPointFormatter;
import com.androidplot.xy.PointLabelFormatter;
import com.androidplot.xy.SimpleXYSeries;
import com.androidplot.xy.XYPlot;
import com.androidplot.xy.XYSeries;
import com.androidplot.xy.XYStepMode;
import com.firebase.client.DataSnapshot;
import com.firebase.client.Firebase;
import com.firebase.client.FirebaseError;
import com.firebase.client.ValueEventListener;

import org.json.JSONObject;

import java.net.InetAddress;
import java.text.DecimalFormat;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Vector;

public class HTSensorActivity extends Activity {

    private static final Constants c = Constants.getInstance();
    private AES128 mAES = new AES128();
    private String mIp;
    private int mPort;
    private String mAESKey;
    private String mFirebaseAppId;
    private String mMac;
    private String mDescription;
    private TextView textViewTemp;
    private TextView textViewHumi;

    private boolean runLocalLink = true;
    private boolean runCloudLink = true;

    private XYPlot plot;
    private List<Double> timeList= new Vector<>();
    private List<Double> temperatureList=new Vector<>();
    private List<Double> humidityList=new Vector<>();
    private int maxListSize = 6;
    private long timeCounter = 0;
    private boolean receivedNewData =false;
    private SharedPreferences sp;
    private int dataUpdateDelay = c.val_ht_sensor_default_data_update_frequency;
    private boolean alarmOptions = c.val_ht_sensor_default_alarm_options;
    private double tempTresh = c.val_ht_sensor_default_temperature_threshold;
    private double humiTresh = c.val_ht_sensor_default_humidity_threshold;
    private String alarmSound = c.val_ht_sensor_default_alarm_sound;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_device_ht);

        mIp=getIntent().getStringExtra(c.key_ip);
        mPort=getIntent().getIntExtra(c.key_port, c.val_device_not_found);
        mMac=getIntent().getStringExtra(c.key_mac);
        mAESKey=getIntent().getStringExtra(c.key_shared_key);
        mFirebaseAppId=getIntent().getStringExtra(c.key_firebase_app_id);
        mDescription=getIntent().getStringExtra(c.key_device_description);

        setTitle(mDescription);

        textViewTemp = (TextView)findViewById(R.id.textViewTemperature);
        textViewHumi = (TextView)findViewById(R.id.textViewHumidity);

        loadSharedPreference();

        if((!mIp.equals(c.val_empty))&&(mPort!=c.val_device_not_found)){
            try {
                amebaLocalLink(mIp, mPort);
            }
            catch (Exception e){
                Log.e(c.tag_ht_sensor_activity,"connectToAmeba() Exception e "+e);
            }
        }
        else {
            amebaCloudLink(mFirebaseAppId);
        }

        plotChart();
    }


    @Override
    protected void onDestroy() {
        super.onDestroy();
        runLocalLink = false;
        runCloudLink = false;
        Log.v(c.tag_ht_sensor_activity, "onDestroy()");
    }


    private void loadSharedPreference(){
        sp = PreferenceManager.getDefaultSharedPreferences(getBaseContext());
        dataUpdateDelay = Integer.parseInt(sp.getString(c.key_ht_sensor_data_update_frequency, Integer.toString(dataUpdateDelay)));
        alarmOptions = sp.getBoolean(c.key_ht_sensor_alarm_options,false);
        tempTresh = Double.parseDouble(sp.getString(c.key_ht_sensor_temperature_threshold, Double.toString(tempTresh)));
        humiTresh=Double.parseDouble(sp.getString(c.key_ht_sensor_humidity_threshold,Double.toString(humiTresh)));
        alarmSound=sp.getString(c.key_ht_sensor_alarm_sound, alarmSound);
    }

    private void playAlarm(double temp, double humi){
        if((alarmOptions)&&((temp>tempTresh)||(humi>humiTresh))){
            String sound = alarmSound;
            MediaPlayer mediaPlayer;

            if(sound.equals(c.val_ht_sensor_alarm_sound_altair)){
                mediaPlayer = MediaPlayer.create(this, R.raw.sound_altair);
            }
            else if(sound.equals(c.val_ht_sensor_alarm_sound_ariel)){
                mediaPlayer = MediaPlayer.create(this,R.raw.sound_ariel);
            }
            else if(sound.equals(c.val_ht_sensor_alarm_sound_fomalhaut)){
                mediaPlayer = MediaPlayer.create(this,R.raw.sound_fomalhaut);
            }
            else if(sound.equals(c.val_ht_sensor_alarm_sound_tinkerbell)){
                mediaPlayer = MediaPlayer.create(this,R.raw.sound_tinkerbell);
            }
            else {
                mediaPlayer = MediaPlayer.create(this,R.raw.sound_ding);
            }

            mediaPlayer.start();
        }
    }

    private void amebaLocalLink(String ip, int port)throws Exception{

        final TcpClient tcpClient = new TcpClient();
        Log.v(c.tag_ht_sensor_activity, "amebaLocalLink() ip/port: " + ip + "/" + port);
        Log.v(c.tag_ht_sensor_activity, "amebaLocalLink() AES key: " + mAESKey);

        final String mIp=ip;
        final int mPort=port;

        class AmebaLocalLinkGetDataThread extends Thread{
            @Override
            public void run() {
                while (runLocalLink){
                    try {
                        Thread.sleep(dataUpdateDelay);
                        Log.v(c.tag_ht_sensor_activity,"amebaLocalLink() get data frequency: Every "+dataUpdateDelay+"ms");
                    }
                    catch (InterruptedException e){Log.e(c.tag_ht_sensor_activity,"AmebaLocalLinkGetDataThread InterruptedException e"+e);}

                    try {
                        String txEnc = mAES.encryptToStringOfShortArray("request", mAESKey);
                        Log.v(c.tag_ht_sensor_activity, "amebaLocalLink() txEnc: " + txEnc);

                        String rxEnc = tcpClient.echo(InetAddress.getByName(mIp), mPort, txEnc);
                        Log.v(c.tag_ht_sensor_activity,"amebaLocalLink() rxEnc: "+rxEnc);

                        String rx = mAES.decryptFromStringOfShortArray(rxEnc,mAESKey);
                        Log.v(c.tag_ht_sensor_activity, "amebaLocalLink() rx: " + rx);


                        ////////////////////////parse json data////////////////////////
                        JSONObject jObject = new JSONObject(rx);
                        final double hum = jObject.getDouble(c.key_humidity);
                        final double tem = jObject.getDouble(c.key_temperature);
                        ////////////////////////parse json data////////////////////////

                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                textViewTemp.setText("" +tem);
                                textViewHumi.setText(""+hum);
                                updateDataList(tem,hum);
                                playAlarm(tem,hum);
                            }
                        });

                    }
                    catch (Exception e){Log.e(c.tag_ht_sensor_activity,"AmebaLocalLinkGetDataThread Exception e"+e);}

                }//while(true)
            }
        }

        new AmebaLocalLinkGetDataThread().start();
    }


    private void amebaCloudLink(String firebaseAppId){

        String firebaseURL = "https://"+firebaseAppId+".firebaseio.com";
        Firebase.setAndroidContext(this);

        final Firebase firebaseRef = new Firebase(firebaseURL);

        Log.v(c.tag_ht_sensor_activity, "amebaCloudLink() connect to firebase: " + firebaseURL);

        firebaseRef.child(c.nsd_service_ht).child(mMac).addValueEventListener(new ValueEventListener() {

            @Override
            public void onDataChange(DataSnapshot snapshot) {
                if (runCloudLink) {

                    Map<String, Object> newData = (Map<String, Object>) snapshot.getValue();

                    try {
                        double hum = Double.parseDouble((String) newData.get(c.key_humidity));
                        double tem = Double.parseDouble((String) newData.get(c.key_temperature));
                        textViewTemp.setText("" + tem);
                        textViewHumi.setText("" + hum);
                        updateDataList(tem, hum);
                        playAlarm(tem, hum);
                        Log.v(c.tag_ht_sensor_activity, snapshot.getValue().toString());
                    } catch (Exception e) {
                        Log.e(c.tag_ht_sensor_activity, "onDataChange() Exception e " + e);
                    }
                }
            }

            @Override
            public void onCancelled(FirebaseError error) {
                Log.e(c.tag_ht_sensor_activity, "onCancelled() FirebaseError error" + error);
            }
        });

    }


    //////////////////////////////////////////////plot//////////////////////////////////////////////

    private void plotChart(){

        //init lists
        timeList.add((double)0);
        temperatureList.add((double)0);
        humidityList.add((double)0);

        new Thread() {
            public void run() {
                while (runLocalLink) {
                    try {
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                //update plot data
                                timeCounter++;
                                timeList.add((double)timeCounter);
                                while (timeList.size()>maxListSize){
                                    timeList.remove(0);
                                }

                                //sync data with time:
                                if(!receivedNewData){
                                    //push data manually
                                    updateDataList(temperatureList.get(temperatureList.size()-1),humidityList.get(humidityList.size()-1));
                                }

                                //sync list length
                                //consider 1.when series size < timelist size 2.when series size > timelist size
                                while (temperatureList.size()<timeList.size()){
                                    temperatureList.add(temperatureList.get(temperatureList.size()-1));
                                }
                                while (humidityList.size()<timeList.size()){
                                    humidityList.add(humidityList.get(humidityList.size()-1));
                                }
                                while (temperatureList.size()>timeList.size()){
                                    temperatureList.remove(0);
                                }
                                while (humidityList.size()>timeList.size()){
                                    humidityList.remove(0);
                                }

                                try{
                                    plot.clear();
                                }
                                catch (Exception e){
                                    Log.e(c.tag_ht_sensor_activity,"Exception e"+e);
                                }

                                //////////////////////////////////////////////////plot setup//////////////////////////////////////////////////
                                // initialize our XYPlot reference:
                                plot = (XYPlot) findViewById(R.id.mySimpleXYPlot);
                                plot.setBorderStyle(XYPlot.BorderStyle.NONE, null, null);
                                plot.setGridPadding(5, 25, 25, 5);
                                plot.getGraphWidget().setSize(new SizeMetrics(0, SizeLayoutType.FILL, 0, SizeLayoutType.FILL));
                                plot.getGraphWidget().getBackgroundPaint().setColor(Color.argb(0, 0, 0, 0));
                                plot.getGraphWidget().getGridBackgroundPaint().setColor(Color.argb(0, 0, 0, 0));

                                //domain -- x
                                plot.getGraphWidget().getDomainLabelPaint().setColor(Color.BLACK);
                                plot.getGraphWidget().getDomainOriginLabelPaint().setColor(Color.BLACK);
                                plot.getGraphWidget().getDomainOriginLinePaint().setColor(Color.BLACK);
                                plot.setDomainValueFormat(new DecimalFormat("0"));
                                plot.setDomainStep(XYStepMode.INCREMENT_BY_VAL, 1);
                                plot.setDomainStepValue(1);

                                //range -- y
                                plot.getGraphWidget().getRangeLabelPaint().setColor(Color.BLACK);
                                plot.getGraphWidget().getRangeOriginLabelPaint().setColor(Color.BLACK);
                                plot.getGraphWidget().getRangeOriginLinePaint().setColor(Color.BLACK);
                                plot.setRangeValueFormat(new DecimalFormat("0"));
                                plot.setRangeStep(XYStepMode.INCREMENT_BY_VAL, 1);
                                plot.setRangeStepValue(5);

                                //label
                                plot.getDomainLabelWidget().getLabelPaint().setColor(Color.BLACK);
                                plot.getRangeLabelWidget().getLabelPaint().setColor(Color.BLACK);


                                // Create a formatter to use for drawing a series using LineAndPointRenderer
                                // and configure it from xml:
                                LineAndPointFormatter seriesHumiFormat = new LineAndPointFormatter();
                                seriesHumiFormat.setPointLabelFormatter(new PointLabelFormatter());
                                seriesHumiFormat.configure(getApplicationContext(), R.xml.line_point_formatter_with_plf1);

                                // same as above:
                                LineAndPointFormatter seriesTempFormat = new LineAndPointFormatter();
                                seriesTempFormat.setPointLabelFormatter(new PointLabelFormatter());
                                seriesTempFormat.configure(getApplicationContext(), R.xml.line_point_formatter_with_plf2);


                                // reduce the number of range labels
                                plot.setTicksPerRangeLabel(3);
                                plot.getGraphWidget().setDomainLabelOrientation(-45);

                                XYSeries seriesHumi = new SimpleXYSeries(timeList, humidityList, "Humidity");
                                XYSeries seriesTemp = new SimpleXYSeries(timeList, temperatureList, "Temperature");
                                plot.addSeries(seriesHumi, seriesHumiFormat);
                                plot.addSeries(seriesTemp, seriesTempFormat);

                                // handle the boundary bug -> define a boundary, DO NOT USE DEFAULT SETTINGS
                                List<Double> readings =new Vector<>();
                                for (double r : temperatureList){
                                    readings.add(r);
                                }
                                for (double r : humidityList){
                                    readings.add(r);
                                }
                                Collections.sort(readings);
                                double min=readings.get(0);
                                double max=readings.get(readings.size()-1);
                                plot.setRangeBoundaries(min,max, BoundaryMode.FIXED);
                                Log.v(c.tag_ht_sensor_activity,"plotChart() set range boundaries ("+min+","+max+")");
                                //////////////////////////////////////////////////plot setup//////////////////////////////////////////////////
                                plot.redraw();
                                receivedNewData=false;
                                Log.v(c.tag_ht_sensor_activity,"plotChart(): List.size:" + timeList.size());
                                Log.v(c.tag_ht_sensor_activity, "plotChart(): temperatureList.end: " + temperatureList.get(temperatureList.size() - 1));
                                Log.v(c.tag_ht_sensor_activity,"plotChart(): humidityList.end: "+humidityList.get(humidityList.size()-1));
                            }
                        });
                        Thread.sleep(c.val_ui_update_period_ms);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }//while()
            }
        }.start();
    }

    private void updateDataList(double temperature, double humidity){

        temperatureList.add(temperature);
        humidityList.add(humidity);

        while(temperatureList.size()>maxListSize){
            temperatureList.remove(0);
        }

        while(humidityList.size()>maxListSize){
            humidityList.remove(0);
        }

        receivedNewData =true;
    }
}
