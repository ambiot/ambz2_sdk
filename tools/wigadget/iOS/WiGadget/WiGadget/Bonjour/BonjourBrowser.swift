//
//  BonjourBrowser.swift
//  bonjour
//
//  Created by WU JINZHOU on 31/7/15.
//  Copyright (c) 2015 WU JINZHOU. All rights reserved.
//

import UIKit

class BonjourBrowser:NSObject,NetServiceBrowserDelegate,NetServiceDelegate {

    var nsb : NetServiceBrowser!
    var serviceList = [NetService]()
    
    let resolve_time_out:TimeInterval = R.bonjour_service_resolve_time_out
    
    func browse (_ serviceName:String,domain:String) {
        Log.v("\(serviceName)")
        self.serviceList.removeAll()
        self.nsb = NetServiceBrowser()
        self.nsb.delegate = self
        self.nsb.searchForServices(ofType: serviceName,inDomain: domain)
    }
    
    func netServiceBrowser(_ aNetServiceBrowser: NetServiceBrowser, didFind aNetService: NetService, moreComing: Bool) {
        self.serviceList.append(aNetService)
        if !moreComing {
            for service in self.serviceList {
                if service.port == R.not_found {
                    Log.v("name: \(service.name) , type: \(service.type)")
                    service.delegate = self
                    service.resolve(withTimeout: resolve_time_out)
                }
            }
        }
    }
    
    func netService(_ sender: NetService, didUpdateTXTRecord data: Data) {
        Log.v("\(sender)")
        sender.resolve(withTimeout: resolve_time_out)
    }
    
    func netServiceDidResolveAddress(_ sender: NetService) {

        var txtRecords = [String:String]()
        
        //class func dictionaryFromTXTRecordData(_ txtData: NSData) -> [String : NSData]
        for (k,v) in NetService.dictionary(fromTXTRecord: sender.txtRecordData()!){
            txtRecords.updateValue(NSString(data: v , encoding: String.Encoding.ascii.rawValue)! as String , forKey: k )
        }
        
        Log.v("TXTRecord:\(txtRecords)")
        
        let ameba = Ameba(resolvedTxtRecords: txtRecords)
        AmebaList.update(ameba)
        
        sender.startMonitoring()

    }
}
