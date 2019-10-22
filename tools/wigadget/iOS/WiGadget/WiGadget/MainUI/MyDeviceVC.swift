//
//  MyDeviceVC.swift
//  WiGadget
//
//  Created by WU JINZHOU on 24/7/15.
//  Copyright (c) 2015 WU JINZHOU. All rights reserved.
//

import UIKit

class MyDeviceVC: UIViewController,UICollectionViewDataSource, UICollectionViewDelegate,SavedListDelegate {

    let bonjourBrowser = BonjourBrowser()
    
    @IBOutlet weak var menuBtn: UIBarButtonItem!
    @IBOutlet weak var collectionView: UICollectionView!
    
    var imageSource = [String]()
    var textSource = [String]()
    var macSource = [String]()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        menuBtn.target = self.revealViewController()
        menuBtn.action = #selector(SWRevealViewController.revealToggle(_:))
        self.view.addGestureRecognizer(self.revealViewController().panGestureRecognizer())
        self.title = R.menu_titles[0]
        
        AmebaList.load()
        AmebaList.savedListDataSource = self

        
        let gesture: UILongPressGestureRecognizer = UILongPressGestureRecognizer(target: self, action: #selector(MyDeviceVC.collectionViewCellLongPressed(_:)))
        gesture.minimumPressDuration = R.device_cell_long_press_duration
        self.collectionView.addGestureRecognizer(gesture)
        
        Log.v("Start Bonjour Service Discovery")
        bonjourBrowser.browse(R.bonjour_service_name, domain: R.bonjour_service_domain)
        
        updateDataSource(AmebaList.savedList)
    }
    
    
    func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        return imageSource.count
    }
    
    
    func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        let cell = collectionView.dequeueReusableCell(withReuseIdentifier: R.device_cell_ids[1], for: indexPath) as! DeviceCollectionViewCell
        cell.myDeviceTextLabel.text = textSource[indexPath.row]
        cell.image = imageSource[indexPath.row]
        cell.myDeviceImageView.image = UIImage(named: cell.image)
        cell.mac = macSource[indexPath.row]
        return cell
    }
    
    
    func collectionView(_ collectionView: UICollectionView, willDisplay cell: UICollectionViewCell, forItemAt indexPath: IndexPath) {
        let selectedView = UIView() as UIView
        selectedView.layer.borderWidth = R.device_cell_on_selected_border_width
        selectedView.layer.cornerRadius = R.device_cell_on_selected_corner_radius;
        selectedView.layer.borderColor = R.device_cell_on_selected_color.cgColor
        selectedView.backgroundColor = R.device_cell_on_selected_color
        cell.selectedBackgroundView = selectedView
    }
    

    func onSavedListUpdated(_ trackingList: [Ameba]) {
        //trackingList updated do something...
        Log.v("updating cell data source")
        updateDataSource(AmebaList.savedList)
        
        DispatchQueue.main.async(execute: {
            self.collectionView.reloadData()
        })

    }
    

    func updateDataSource(_ amebaList:[Ameba]) {
        imageSource.removeAll(keepingCapacity: false)
        textSource.removeAll(keepingCapacity: false)
        macSource.removeAll(keepingCapacity: false)
        
        for ameba in amebaList {
            // TODO: add code for new device here
            if ameba.name == R.ht_sensor {

                // cell image
                imageSource.append(R.ic_ht_sensor[ameba.control_type]![ameba.link_state]!)
                
                // cell txt label
                textSource.append(ameba.description)
                
                // cell mac
                macSource.append(ameba.mac)
            }
        }
        
    }
    
    func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
        collectionView.deselectItem(at: indexPath, animated: true)
        let cell = collectionView.cellForItem(at: indexPath) as! DeviceCollectionViewCell
        Log.v("cell \(indexPath.row) , mac \(cell.mac) selected")
        
        //get ameba from saved list
        if let ameba = AmebaList.savedListGetAmebaByMac(cell.mac) {
            
            //check connection first
            var deviceOnline = false
            
            //let qos = DispatchQoS.QoSClass.userInitiated.rawValue
            let mainQ = DispatchQueue.main
            let networkingQ = DispatchQueue.global(qos: .background)
            
            self.view.makeToastActivity()
            networkingQ.async{
                // do something off-UI
                if ameba.control_type == R.cloud_control {
                    let code = Just.get(self.fmtURL(ameba.firebase_app_id)).statusCode ?? R.not_found
                    if code == 200 {
                        deviceOnline = true
                        Log.v("cloud link ready")
                    }
                    else{
                        Log.v("cloud link is not avaliable, status code: \(code)")
                    }
                }
                
                if let trackedAmeba = AmebaList.trackingListGetAmebaByMac(ameba.mac) {
                    
                    let connIp = trackedAmeba.ip
                    let connPort = Int(trackedAmeba.port)!
                    let client = TCPClient(address: connIp, port: Int32(connPort))
                    let ret = client.connect(timeout: R.tcp_connection_time_out)
                    if !ret.isSuccess {
                        Log.v("local link is not avaliable connection err: \(ret.error)")
                    }
                    else{
                        Log.v("local link is ready")
                        deviceOnline = true
                    }
                    client.close()
                    
                }
                
                mainQ.async{
                    // switch back to main Q
                    self.view.hideToastActivity()
                    
                    if deviceOnline {

                        ameba.link_state = R.online
                        AmebaList.savedListAddAmeba(ameba)
                        
                        AmebaList.linkTarget = ameba

                        //TODO: add code for new device here
                        if ameba.name == R.ht_sensor {
                            let htVC = UIStoryboard(name: R.storybord_name_ht,bundle:nil).instantiateViewController(withIdentifier: R.storyboard_init_ht_vc[R.storybord_name_ht]!)
                            self.present(htVC, animated: true, completion: nil)
                        }
                    }
                    else {
                        self.view.makeToast("Link connection error\nDevice is offline", duration: R.toast_duration, position: CSToastPositionBottom, title: "CONNECTION FAILED", image: R.toast_ic_failed)
                        ameba.link_state = R.offline
                        AmebaList.savedListAddAmeba(ameba)
                    }
                }
            }

        }
        
    }
    
    func collectionViewCellLongPressed(_ longPress: UIGestureRecognizer) {
        
        if longPress.state == UIGestureRecognizerState.began {
            
            let point = longPress.location(in: self.collectionView)
            let indexPath = self.collectionView.indexPathForItem(at: point)
            
            if let index = indexPath {
                let cell = collectionView.cellForItem(at: index) as! DeviceCollectionViewCell
                Log.v("cell \(index.row) , mac \(cell.mac) long-pressed")
                
                //get ameba info
                if let ameba = AmebaList.savedListGetAmebaByMac(cell.mac){
                    
                    let alertTitle = "Device Options"
                    let amebaInfo = ameba.name + "\nDescription: " + ameba.description + "\nControl Type: " + ameba.control_type
                    var alert = SCLAlertView()
                    alert.showAnimationType = .SlideInToCenter
                    alert.hideAnimationType = .SlideOutToCenter
                    alert.backgroundType = .Blur
                    
                    alert.addButton("Rename Device", actionBlock: { () -> Void in
                        Log.v("Rename Device is clicked")
                        
                        alert = SCLAlertView()
                        alert.showAnimationType = .SlideInToCenter
                        alert.hideAnimationType = .SlideOutToCenter
                        alert.backgroundType = .Transparent
                        alert.removeTopCircle()
                        let renameTextField = alert.addTextField("New Name")
                        alert.addButton("Done", validationBlock: { () -> Bool in
                            var name = renameTextField?.text! ?? R.not_assigned
                            name = name.removeWhitespace()
                            let nameInChar = name.characters.map({$0})
                            if nameInChar.count == 0 {
                                
                                self.view.makeToast("Rename failed\nThe new name should not be empty", duration: R.toast_duration, position: CSToastPositionBottom, title: "RENAME FAILED", image: R.toast_ic_failed)
                                Log.v("rename failed : new name is not avaliable")
                                
                                return false
                            }
                            return true
                            
                        }, actionBlock: { () -> Void in
                            ameba.description = (renameTextField?.text!)!
                            AmebaList.savedListAddAmeba(ameba)
                            
                            self.view.makeToast("Rename succeed\nThe new name is saved", duration: R.toast_duration, position: CSToastPositionBottom, title: "RENAME SUCCEED", image: R.toast_ic_succeed)

                            Log.v("rename success : new name: \(ameba.description)")
                        })
                        
                        alert.showCustom(self, image: UIImage(named: cell.image), color: R.device_cell_on_selected_color, title: "Rename Device", subTitle: amebaInfo, closeButtonTitle: "Cancel", duration: 0)
                        
                    })
                    
                    alert.addButton("Remove Device", actionBlock: { () -> Void in
                        Log.v("Remove Device is clicked")
                        
                        //let qos = DispatchQoS.QoSClass.userInitiated.rawValue
                        let mainQ = DispatchQueue.main
                        let sockQ = DispatchQueue.global(qos: .background)
                        
                        var unpairResult = R.unpair_failed_on_unknown_err
                        
                        self.view.makeToastActivity()
                        sockQ.async{
                            // do something off-UI
                            unpairResult = self.unpair(ameba)
                            
                            mainQ.async{
                                // switch back to main Q
                                self.view.hideToastActivity()
                                if unpairResult == R.unpair_success {
                                    self.view.makeToast("Remove device succeed\nDevice unpaired", duration: R.toast_duration, position: CSToastPositionBottom, title: "UNPAIR SUCCESS", image: R.toast_ic_succeed)
                                }
                                else{
                                    self.view.makeToast("Remove device failed\nError code: \(unpairResult)", duration: R.toast_duration, position: CSToastPositionBottom, title: "UNPAIR Failed", image: R.toast_ic_failed)
                                    
                                    //force to remove device
                                    let alert = SCLAlertView()
                                    alert.showAnimationType = .SlideInToCenter
                                    alert.hideAnimationType = .SlideOutToCenter
                                    alert.backgroundType = .Transparent
                                    alert.removeTopCircle()
                                    
                                    alert.addButton("Remove", actionBlock: { () -> Void in
                                        Log.v("Force Remove Device is clicked")
                                        
                                        AmebaList.unpair(ameba)
                                        
                                    })
                                    
                                    let subTitle = "Force this device to be removed?"
                                    
                                    alert.showCustom(self, image: UIImage(named: cell.image), color: R.device_cell_on_selected_color, title: "Force Remove", subTitle: subTitle, closeButtonTitle: "Cancel", duration: 0 as TimeInterval)
                                    
                                    //force remove done
                                }
                            }
                        }
                        
                    })

                    alert.showCustom(self, image: UIImage(named: cell.image), color: R.device_cell_on_selected_color, title: alertTitle, subTitle: amebaInfo, closeButtonTitle: "Cancel", duration: R.scl_alert_auto_dismiss_time)
                }
                
            }
            else {
                Log.e("Could not find cell")
            }
        }
        
    }
    
    func unpair(_ ameba:Ameba) -> Int {
        if let trackedAmeba = AmebaList.trackingListGetAmebaByMac(ameba.mac) {
            
            let connIp = trackedAmeba.ip
            let connPort = Int(trackedAmeba.port)!
            let sharedKey = ameba.key
            var rxData: [UInt8]?
            
            //setup connection
            let client = TCPClient(address: connIp, port: Int32(connPort))
            var ret = client.connect(timeout: R.tcp_connection_time_out)
            if !ret.isSuccess {
                client.close()
                Log.e(ret.error)
                return R.unpair_failed_connection_time_out
            }
            
            //send unpair cmd
            let cipherUnpairCmd = Crypto.encrypt(R.tcp_tx_unpair, key: sharedKey)
            ret = client.send(string: cipherUnpairCmd)
            if !ret.isSuccess {
                client.close()
                Log.e(ret.error)
                return R.unpair_failed_on_send_cmd
            }
            
            rxData = client.read(R.tcp_expect_rx_data_length)!
            
            if let data = rxData {
                let rx = String(bytes: data, encoding: String.Encoding.utf8)!
                
                if rx == R.tcp_rx_error {
                    client.close()
                    Log.e("unpair_failed_cmd_rejected_by_server")
                    return R.unpair_failed_cmd_rejected_by_server
                }
                
                if rx == R.tcp_rx_unpair_success {
                    
                    //remove device success
                    
                    AmebaList.unpair(ameba)
                    
                    client.close()
                    Log.i("tcp_rx_unpair_success")
                    return R.unpair_success
                }
                
            }
            else{
                client.close()
                Log.e("unpair_failed_rx")
                return R.unpair_failed_on_rx
            }
            
            client.close()
            Log.e("unpair_failed_on_unknown_err")
            return R.unpair_failed_on_unknown_err
            
        }
        Log.e("unpair_failed_device_not_found")
        return R.unpair_failed_device_not_found
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
    
    func fmtURL(_ firebaseAppId:String) -> String {
        
        var firebaseUrl = ""
        
        if firebaseAppId.hasSuffix(".firebaseio.com") {
            firebaseUrl = "https://" + firebaseAppId + "/"
        }
        else {
            firebaseUrl = "https://" + firebaseAppId + ".firebaseio.com/"
        }
        
        firebaseUrl = firebaseUrl.removeWhitespace().lowercased()
        
        return firebaseUrl
    }
    
    /*
    // MARK: - Navigation

    // In a storyboard-based application, you will often want to do a little preparation before navigation
    override func prepareForSegue(segue: UIStoryboardSegue, sender: AnyObject?) {
        // Get the new view controller using segue.destinationViewController.
        // Pass the selected object to the new view controller.
    }
    */

}

extension String {
    func replace(_ string:String, replacement:String) -> String {
        return self.replacingOccurrences(of: string, with: replacement, options: NSString.CompareOptions.literal, range: nil)
    }
    
    func removeWhitespace() -> String {
        return self.replace(" ", replacement: "")
    }
}
