package com.realtek.wigadget;

/**
 * Created by Wu Jinzhou on 4/21/2015.
 */
public final class Constants {
    private static Constants thisConstants;
    public static synchronized Constants getInstance(){
        if (thisConstants==null){
            thisConstants = new Constants();
        }
        return thisConstants;
    }
    private Constants(){
    }

    ///////////////////////////////////////device list///////////////////////////////////////
    String key_mac = "MAC_ADDR";
    String key_date_format = "yyyy-MM-dd HH:mm:ss";
    String key_ip = "IP";
    String key_port = "PORT";
    String key_service_name = "SERVICE_NAME";
    String key_control_type = "CONTROL_TYPE";
    String key_shared_key = "SHARED_KEY";
    String key_device_description = "DESCRIPTION";
    String key_date = "DATE";
    String key_firebase_app_id = "FIREBASE_APP_ID";
    String key_pair_state = "PAIR_STATE";
    String val_file_name = "PairedDevices.json";
    String val_local_control = "0";
    String val_cloud_control = "1";
    String val_not_paired = "0";
    String val_paired = "1";
    String val_empty = "";
    int val_device_not_found = -1;
    ///////////////////////////////////////device list///////////////////////////////////////

    ///////////////////////////////////////nsd///////////////////////////////////////
    String nsd_service_type = "_Ameba._tcp.local.";
    String nsd_service_ht = "ht_sensor";
    String nsd_service_unknown = "unknown";
    ///////////////////////////////////////nsd///////////////////////////////////////

    ///////////////////////////////////////tags///////////////////////////////////////
    String tag_about_fragment = "@AboutFragment ";
    String tag_nsd_helper = "@NsdHelper ";
    String tag_device_list = "@DeviceList ";
    String tag_find_device_fragment = "@FindDeviceFragment ";
    String tag_main_activity = "@MainActivity ";
    String tag_tcp_client = "@TcpClient ";
    String tag_key_gen = "@KeyGen ";
    String tag_my_device_fragment = "@MyDeviceFragment ";
    String tag_ht_sensor_activity = "@HTSensorActivity ";
    String tag_connection_checker ="@ConnectionChecker ";
    ///////////////////////////////////////tags///////////////////////////////////////

    ////////////////////////////////////////tcp///////////////////////////////////////
    String tcp_tx_pair = "PAIR";
    String tcp_tx_cmd_unpairing = "remove";
    String tcp_rx_unpairing_ok = "Remove OK";
    String tcp_rx_error = "ERROR";
    String tcp_rx_firebase_app_url = "FIREBASE URL";
    String tcp_rx_pair_ok = "PAIR OK";
    int val_tcp_next_cmd_waiting_ms = 500; //ameba side delay: 250ms
    ///////////////////////////////////////tcp////////////////////////////////////////

    ///////////////////////////////////////pairing///////////////////////////////////////
    int val_pairing_succeed = 0;
    int err_pairing_failed_to_start = -1;
    int err_server_no_response = -2;
    int err_pair_cmd_rejected = -3;
    int err_public_key_rejected = -4;
    int err_firebase_app_url_rejected = -5;
    int err_unknown_host_exception = -6;
    int err_interrupted_exception = -7;
    int err_io_exception = -8;
    int err_firebase_app_url_cannot_be_verified = -9;
    int err_unknown_error = -10;
    ///////////////////////////////////////pairing///////////////////////////////////////

    ///////////////////////////////////////unpairing///////////////////////////////////////
    int val_unpairing_succeed = 0;
    int err_unpairing_device_not_found = -1;
    int err_unpairing_server_ping_timeout = -2;
    int err_unpairing_with_server_failed = -3;
    ///////////////////////////////////////unpairing///////////////////////////////////////

    ///////////////////////////////////////firebase///////////////////////////////////////
    String val_firebase_register_url = "https://www.firebase.com/signup/";
    String key_humidity = "HUM";
    String key_temperature = "TEM";
    ///////////////////////////////////////firebase///////////////////////////////////////

    //////////////////////////////////ht sensor activity//////////////////////////////////
    String key_ht_sensor_data_update_frequency = "data_update_frequency";
    String key_ht_sensor_alarm_options = "ht_alarm_options";
    String key_ht_sensor_temperature_threshold = "temperature_threshold";
    String key_ht_sensor_humidity_threshold = "humidity_threshold";
    String key_ht_sensor_alarm_sound = "alarm_sound";
    int val_ht_sensor_default_data_update_frequency = 2000;
    boolean val_ht_sensor_default_alarm_options = false;
    double val_ht_sensor_default_temperature_threshold = 100.0;
    double val_ht_sensor_default_humidity_threshold = 100.0;
    String val_ht_sensor_default_alarm_sound = "sound_ding";
    String val_ht_sensor_alarm_sound_altair = "sound_altair";
    String val_ht_sensor_alarm_sound_ariel = "sound_ariel";
    String val_ht_sensor_alarm_sound_fomalhaut = "sound_fomalhaut";
    String val_ht_sensor_alarm_sound_tinkerbell = "sound_tinkerbell";
    //////////////////////////////////ht sensor activity//////////////////////////////////

    ///////////////////////////////////////ping///////////////////////////////////////
    int val_ping_base_time_out = 500;
    int val_ping_max_try = 4;
    ///////////////////////////////////////ping///////////////////////////////////////

    ///////////////////////////////////////ui///////////////////////////////////////
    int val_ui_update_period_ms = 1000;
    int val_ui_double_click_interval = 500;
    ///////////////////////////////////////ui///////////////////////////////////////

}
