//
//  AmebaDevice.swift
//  bonjour
//
//  Created by WU JINZHOU on 11/8/15.
//  Copyright (c) 2015 WU JINZHOU. All rights reserved.
//

import Foundation

class Ameba {
    
    var name:String = R.not_assigned
    var ip:String = R.not_assigned
    var mac:String = R.not_assigned
    var port:String = R.not_assigned
    var control_type:String = R.not_assigned
    var pair_state:String = R.not_assigned
    
    var link_state:String = R.offline
    var firebase_app_id:String = R.not_assigned
    var key:String = R.not_assigned
    var description:String = R.not_assigned

    init(resolvedTxtRecords:[String:String]){
        self.name = resolvedTxtRecords[R.key_service_name]!
        self.mac = resolvedTxtRecords[R.key_service_mac]!
        self.ip = resolvedTxtRecords[R.key_service_ip]!
        self.port = resolvedTxtRecords[R.key_service_port]!
        self.control_type = resolvedTxtRecords[R.key_service_control_type]!
        self.pair_state = resolvedTxtRecords[R.key_service_pair_state]!
    }
    
    init(device_mac:String,service_name:String,control_type:String,shared_key:String,firebase_app_id:String,description:String,pair_state:String,link_state:String){
        self.mac = device_mac
        self.name = service_name
        self.control_type = control_type
        self.key = shared_key
        self.firebase_app_id = firebase_app_id
        self.description = description
        self.pair_state = pair_state
        self.link_state = link_state
    }
    
    func printInfo(){
        Log.i("* name: \(name) ip: \(ip) mac: \(mac) port: \(port) control_type: \(control_type) pair_state: \(pair_state) link_state: \(link_state) firebase_app_id: \(firebase_app_id) description: \(description) key: \(key)")
    }
}
