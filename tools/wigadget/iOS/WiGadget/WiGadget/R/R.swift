//
//  R.swift
//  WiGadget
//
//  Created by WU JINZHOU on 23/7/15.
//  Copyright (c) 2015 WU JINZHOU. All rights reserved.
//

import Foundation

class R {
    
    //MARK: Table View - The Slide menu cells
    static let menu_titles = ["My Devices","Find Device","Settings","About"]
    static let menu_label_corner_radius:CGFloat = 5
    static let menu_label_trailing = "        " //8 spaces
    
    
    //MARK: Bonjour service
    static let bonjour_service_name = "_Ameba._tcp."
    static let bonjour_service_domain = "local."
    static let key_service_name = "SERVICE_NAME"
    static let key_service_port = "PORT"
    static let key_service_ip = "IP"
    static let key_service_mac = "MAC_ADDR"
    static let key_service_control_type = "CONTROL_TYPE"
    static let key_service_pair_state = "PAIR_STATE"
    static let key_service_firebase_app_id = "FIREBASE_APP_ID" //not in the origional txt record
    static let key_service_shared_key = "SHARED_KEY" //not in the origional txt record
    static let key_service_description = "DEVICE_DESCRIPTION" //not in the origional txt record
    static let key_link_state = "LINK_STATE" //not in the origional txt record
    static let bonjour_service_resolve_time_out:TimeInterval = 10
    static let paired = "1"
    static let not_paired = "0"
    static let cloud_control = "1"
    static let local_control = "0"
    static let online = "1"
    static let offline = "0"
    
    //MARK: The initial values
    static let debug_message_enable = true
    static let debug_message_in_color = false
    static let curve25519_base_point:[UInt8] = [9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    static let aes_iv = "AAAAAAAAAAAAAAAA"
    static let not_found = -1 //DO NOT change this
    static let not_assigned = "" //DO NOT change this
    static let tcp_connection_time_out = 3
    static let tcp_expect_rx_data_length = 10240
    static let tcp_thread_sleep_time:TimeInterval = 0.5 //sleep 500 ms, ameba side seleeps 250 ms
    static let web_page_load_timeout:TimeInterval = 20.0
    
    //MARK: User Defaults & JSON
    static let key_ameba_data = "AMEBA_DATA"
    
    //MARK: TCP handshake & commands
    static let pair_success = 0
    static let pair_failed_on_first_hs_tx = -1
    static let pair_failed_on_first_hs_rx = -2
    static let pair_failed_on_second_hs_tx = -3
    static let pair_failed_on_second_hs_rx = -4
    static let pair_failed_on_third_hs_tx = -5
    static let pair_failed_on_third_hs_rx = -6
    static let pair_failed_on_verify_firebase_url = -7
    static let pair_failed_connection_time_out = -8
    static let pair_failed_cmd_rejected_by_server = -9
    static let pair_failed_pk_rejected_by_server = -10
    static let pair_failed_firebase_id_rejected_by_server = -11
    static let pair_failed_on_unknown_err = -12
    static let unpair_success = 0
    static let unpair_failed_device_not_found = -1
    static let unpair_failed_connection_time_out = -2
    static let unpair_failed_on_send_cmd = -3
    static let unpair_failed_on_rx = -4
    static let unpair_failed_cmd_rejected_by_server = -5
    static let unpair_failed_on_unknown_err = -6
    static let tcp_tx_pair = "PAIR"
    static let tcp_tx_unpair = "remove"
    static let tcp_tx_request = "request"
    static let tcp_rx_firebase_app_id = "FIREBASE URL"
    static let tcp_rx_error = "ERROR"
    static let tcp_rx_pair_success = "PAIR OK"
    static let tcp_rx_unpair_success = "Remove OK"
    
    //MARK: HTSensor class
    static let ht_readings_to_keep_in_buffer = 10
    static let ht_key_humi = "HUM"
    static let ht_key_temp = "TEM"
    static let ht_chart_description = "H&T Sensor Readings"
    static let ht_chart_no_data_text = "Waiting for H&T sensor data..."
    static let ht_chart_legend_font = UIFont(name: "HelveticaNeue-Light", size:10)!
    static let ht_chart_legend_text_color = UIColor.black
    static let ht_chart_grid_background_color = UIColor.white
    static let ht_chart_color_green_H = UIColor(red: 139/255, green: 179/255, blue: 111/255, alpha: 1) //green -> H
    static let ht_chart_color_blue_T = UIColor(red: 68/255, green: 202/255, blue: 209/255, alpha: 1) //blue -> T
    static let ht_chart_line_width:CGFloat = 1
    static let ht_chart_space_betweenLabels = 1
    static let ht_chart_circle_radius:CGFloat = 3
    static let ht_chart_fill_alpha:CGFloat = 65/255
    static let ht_chart_time_format = "hh:mm:ss a" //format like "11:07:33 PM"
    
    //MARK: Collection View - FindDevice & MyDevice cells
    static let device_cell_ids = ["cell-find-device","cell-my-device"]
    
    //TODO: Add code for new device here
    static let ht_sensor = "ht_sensor"
    static let ic_ht_sensor = [
        cloud_control:
            [online:"ic_device_ht_sensor_cloud_online_128x128",offline:"ic_device_ht_sensor_cloud_offline_128x128"],
        local_control:
            [online:"ic_device_ht_sensor_local_online_128x128",offline:"ic_device_ht_sensor_local_offline_128x128"]
    ]
    static let device_cell_on_selected_border_width:CGFloat = 1
    static let device_cell_on_selected_corner_radius:CGFloat = 10
    static let device_cell_on_selected_color =  UIColor(red: 0/255, green: 176/255, blue: 254/255, alpha: 0.5)
    static let device_cell_long_press_duration:CFTimeInterval = 1
    static let scl_alert_auto_dismiss_time:TimeInterval = 10
    
    //MARK: Storyboard IDs
    static let storybord_name_ht = "HTSensor"
    static let storyboard_init_ht_vc = [storybord_name_ht:"ht-story-board"]
    static let storyboard_name_regi_fb = "RegisterFirebase"
    static let storyboard_init_regi_fb_vc = [storyboard_name_regi_fb:"regi-firebase-story-board"]
    
    //MARK: Settings section
    static let settings_tv_cell_height:CGFloat = 44
    static let settings_key_ht = "SETTINGS_HT"
    static let settings_key_ht_enable_alarm = "USE_ALARM"
    static let settings_key_ht_alarm_sound = "ALARM_SOUND"
    static let settings_key_ht_humi_thres = "HUMIDITY_THRESOLD"
    static let settings_key_ht_temp_thres = "TEMPERATURE_THRESOLD"
    static let settings_key_ht_link_freq = "LINK_FREQUENCY"
    static let settings_ht_alarm_sounds = ["sound_one","sound_two","sound_three"]
    
    //MARK: Toast Message
    static let toast_duration:TimeInterval = 5.0
    static let toast_ic_failed = UIImage(named: "ic_failed_512x513")
    static let toast_ic_succeed = UIImage(named: "ic_succeed_512x513")
    
    
    //MARK: About section
    static let app_build_date = "Sat 26 Sep 2015"
    
}
