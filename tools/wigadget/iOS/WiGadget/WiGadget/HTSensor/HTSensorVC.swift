//
//  HTSensorVC.swift
//  WiGadget
//
//  Created by WU JINZHOU on 2/9/15.
//  Copyright (c) 2015 WU JINZHOU. All rights reserved.
//

import UIKit
import Firebase
import AVFoundation


//ameba in tracking list is like:
/*
[fg0,131,18;* name: ht_sensor ip: 172.25.23.91 mac: 00e04c870000 port: 6866 control_type: 1 pair_state: 1 link_state: 0 firebase_app_id:  description:  key: [;
*/

//ameba in saved list is like:
/*
[fg0,131,18;* name: ht_sensor ip:  mac: 00e04c870000 port:  control_type: 1 pair_state: 1 link_state: 1 firebase_app_id: xxxx description: [NEW]-ht_sensor key: [223, 97, 174, 104, 227, 235, 251, 119, 103, 99, 38, 112, 4, 174, 16, 204][;
*/

//i.e: ip & port are from tracked ameba and key is from saved ameba

class HTSensorVC: UIViewController {
    
    let targetAmeba = AmebaList.linkTarget! //targetAmeba is retrived from saved list
    
    var ip = R.not_assigned
    var port = R.not_found
    var key = R.not_assigned
    var firebaseURL = R.not_assigned
    
    var timer = Timer()
    
    var rootRef = Firebase()
    
    struct Reading {
        var humidity:Double
        var temperature:Double
    }
    
    var readings = [Reading]()
    var times = [String]()
    
   
    @IBOutlet weak var temperatureLabel: UILabel!
    
    @IBOutlet weak var humidityLabel: UILabel!
    
    @IBOutlet weak var chartView: LineChartView!
    
    @IBOutlet weak var imageView: UIImageView!
    
    var preloaderImageData = try? Data(contentsOf: Bundle.main.url(forResource: "preloader_256x23", withExtension: "gif")!)
    
    var useAlarm = false
    var alarmSound = 0 //mapped to sound 1
    var humiThres = 0.0
    var tempThres = 0.0
    var linkFreq = 2.0
    
    var player = AVAudioPlayer()

    override func viewDidLoad() {
        super.viewDidLoad()
        self.title = targetAmeba.description
        imageView.image = UIImage.animatedImage(withAnimatedGIFData: preloaderImageData)

        loadSettings()
        initChart()
    }

    override func viewDidAppear(_ animated: Bool) {
        
        if let trackedAmeba = AmebaList.trackingListGetAmebaByMac(targetAmeba.mac) {
            
            ip = trackedAmeba.ip
            port = Int(trackedAmeba.port)!
            key = targetAmeba.key
            
            timer = Timer.scheduledTimer(timeInterval: linkFreq, target: self, selector: #selector(HTSensorVC.localLink), userInfo: nil, repeats: true)
        }
        
        else if targetAmeba.control_type == R.cloud_control {
            
            firebaseURL = "https://" + targetAmeba.firebase_app_id + ".firebaseio.com"
            rootRef = Firebase(url: firebaseURL)
            
            cloudLink()
        }
        
    }
    
    func loadSettings() {
        let defaults = UserDefaults.standard
        let dataStr = defaults.value(forKey: R.settings_key_ht) as? String ?? R.not_assigned
        let htSettings = JSON(data: dataStr.data(using: String.Encoding.utf8, allowLossyConversion: false)!)
        
        if !(htSettings.isEmpty) {
            useAlarm = htSettings[R.settings_key_ht_enable_alarm].boolValue
            alarmSound = htSettings[R.settings_key_ht_alarm_sound].intValue
            humiThres = htSettings[R.settings_key_ht_humi_thres].doubleValue
            tempThres = htSettings[R.settings_key_ht_temp_thres].doubleValue
            linkFreq = htSettings[R.settings_key_ht_link_freq].doubleValue as TimeInterval
        }
    }
    
    func initChart() {
        chartView.clearsContextBeforeDrawing = true
        let description = Description()
        description.text = R.ht_chart_description
        chartView.chartDescription = description
        chartView.noDataText = R.ht_chart_no_data_text
        //chartView.highlightEnabled = true
        chartView.dragEnabled = false
        chartView.setScaleEnabled(false)
        chartView.drawGridBackgroundEnabled = true
        chartView.pinchZoomEnabled = false
        chartView.legend.font = R.ht_chart_legend_font
        chartView.legend.textColor = R.ht_chart_legend_text_color
        chartView.gridBackgroundColor = R.ht_chart_grid_background_color
        
        let xAxis = chartView.xAxis
        xAxis.labelFont = R.ht_chart_legend_font
        xAxis.labelTextColor = R.ht_chart_legend_text_color
        xAxis.drawGridLinesEnabled = false
        xAxis.drawAxisLineEnabled = false
        //xAxis.spaceBetweenLabels = R.ht_chart_space_betweenLabels
        xAxis.drawGridLinesEnabled = true
        xAxis.labelPosition = .bottom
    }
    
    func handleAlarm(temp:Double, humi:Double, soundIdx:Int) {
        if useAlarm {
            if (temp >= tempThres) || (humi >= humiThres) {
                let path = Bundle.main.path(forResource: R.settings_ht_alarm_sounds[soundIdx], ofType: "wav")
                try! player = AVAudioPlayer(contentsOf: URL(fileURLWithPath: path!))
                player.prepareToPlay()
                player.play()
            }
        }
    }
    
    func cloudLink() {
        
        rootRef.observe(.value, with: {
            snapshot in
            Log.i("cloudLink received:\n\(snapshot?.value)")
            
            let tem = ((((snapshot?.value as? NSDictionary)?[R.ht_sensor] as? NSDictionary)?[self.targetAmeba.mac] as? NSDictionary)?[R.ht_key_temp] as? NSString)!.doubleValue
            let hum = ((((snapshot?.value as? NSDictionary)?[R.ht_sensor] as? NSDictionary)?[self.targetAmeba.mac] as? NSDictionary)?[R.ht_key_humi] as? NSString)!.doubleValue
            
            let reading = Reading(humidity: hum, temperature: tem)
            self.pushData(reading)
            self.handleAlarm(temp: tem, humi: hum, soundIdx: self.alarmSound)
            
            }, withCancel: {
                (error: Error?) in
                Log.e(error)
        })
        
    }
    
    func localLink() {

        //let qos = DispatchQoS.QoSClass.userInitiated.rawValue
        let mainQ = DispatchQueue.main
        let sockQ = DispatchQueue.global(qos: .background)
        var dataStr = R.not_assigned
        var needPushData = false
        
        sockQ.async{
            //off-UI Q
            
            let client = TCPClient(address: self.ip, port: Int32(self.port))
            var ret = client.connect(timeout: R.tcp_connection_time_out)
            
            if !ret.isSuccess {
                Log.e(ret.error)
            }
            else
            {
                let txEnc = Crypto.encrypt(R.tcp_tx_request, key: self.key)
                ret = client.send(string: txEnc)
                if !ret.isSuccess {
                    Log.e(ret.error)
                }
                else
                {
                    if let rxDataEnc = client.read(R.tcp_expect_rx_data_length) {
                        
                        let rxStrEnc = String(bytes: rxDataEnc, encoding: String.Encoding.utf8)!
                        
                        dataStr = Crypto.decrypt(rxStrEnc, key: self.key)
                        
                        if dataStr == R.not_assigned {
                            Log.e("tcp_rx_nothing, maybe use wrong key or send unknown cmd")
                        }
                        else{
                            Log.i("localLink received:\n\(dataStr)")
                            needPushData = true
                        }
                    }
                    else{
                        Log.e("loca_link_rx_nil, read data from server failed")
                    }
                    
                }
            }
            
            client.close()

            mainQ.async
            {
                //main Q
                if needPushData {
                    let json = JSON(data: dataStr.data(using: String.Encoding.utf8, allowLossyConversion: false)!)
                    let humi = (json[R.ht_key_humi].stringValue as NSString).doubleValue
                    let temp = (json[R.ht_key_temp].stringValue as NSString).doubleValue
                    let reading = Reading(humidity: humi, temperature: temp)
                    self.pushData(reading)
                    self.handleAlarm(temp: temp, humi: humi, soundIdx: self.alarmSound)
                }
            }
        }
        
    }

    @IBAction func closeVC(_ sender: AnyObject) {
        
        //...close any link connection
        rootRef.removeAllObservers()
        timer.invalidate()
        self.dismiss(animated: true, completion: nil);
        
    }
    
    func pushData(_ reading:Reading) {
        
        let time = systemTime()
        
        Log.v("pushData: hum: \(reading.humidity) tem: \(reading.temperature) time: \(time)")
        
        readings.append(reading)
        times.append(time)
        
        while (readings.count > R.ht_readings_to_keep_in_buffer) && (readings.count != 0) {
            readings.remove(at: 0)
            times.remove(at: 0)
        }
        
        plotChart()
        temperatureLabel.text = "Temperature:  \(reading.temperature)"
        humidityLabel.text = "Humidity:  \(reading.humidity)"
    }
    
    func systemTime() -> String {
        let date = Date()
        let formatter = DateFormatter()
        formatter.timeZone = TimeZone.autoupdatingCurrent
        formatter.dateFormat = R.ht_chart_time_format
        
        return formatter.string(from: date)
    }
    
    func plotChart() {
        
        let humi = readings.map({$0.humidity})
        let temp = readings.map({$0.temperature})
        var ht = (humi + temp)
        ht.sort(by: <)
        
        let leftAxis = chartView.leftAxis
        leftAxis.labelTextColor = R.ht_chart_color_green_H
        //leftAxis.customAxisMin = floor(ht.first! - (abs(ht.first!) * 0.1))
        //leftAxis.customAxisMax = ceil(ht.last! + (abs(ht.last!) * 0.1))
        leftAxis.drawGridLinesEnabled = true
        //leftAxis.startAtZeroEnabled = false
        
        let rightAxis = chartView.rightAxis
        rightAxis.labelTextColor = R.ht_chart_color_blue_T
        //rightAxis._customAxisMin = floor(ht.first! - (abs(ht.first!) * 0.1))
        //rightAxis.customAxisMax = ceil(ht.last! + (abs(ht.last!) * 0.1))
        rightAxis.drawGridLinesEnabled = true
        //rightAxis.startAtZeroEnabled = false
        
        var hEntry = [ChartDataEntry]()
        for i in 0 ..< readings.count {
            //let dataEntry = ChartDataEntry(x: humi[i], y: Double(i), data: times[i] as AnyObject?)
            let dataEntry = ChartDataEntry(x: Double(i), y: humi[i], data: times[i] as AnyObject?)
            hEntry.append(dataEntry)
        }
        
        let hSet = LineChartDataSet(values: hEntry, label: "Humidity: " + String(format: "%.2f", humi.last!))
        hSet.lineWidth = R.ht_chart_line_width
        hSet.circleRadius = R.ht_chart_circle_radius
        hSet.fillAlpha = R.ht_chart_fill_alpha
        hSet.setColor(R.ht_chart_color_green_H)
        hSet.fillColor = R.ht_chart_color_green_H
        hSet.circleColors = [R.ht_chart_color_green_H]
        
        
        var tEntry = [ChartDataEntry]()
        
        for i in 0 ..< readings.count {
            //let dataEntry = ChartDataEntry(x: temp[i], y: Double(i), data: times[i] as AnyObject?)
            let dataEntry = ChartDataEntry(x: Double(i), y: temp[i], data: times[i] as AnyObject?)
            tEntry.append(dataEntry)
        }
        
        let tSet = LineChartDataSet(values: tEntry, label: "Temperature: " + String(format: "%.2f", temp.last!))
        tSet.lineWidth = R.ht_chart_line_width
        tSet.circleRadius = R.ht_chart_circle_radius
        tSet.fillAlpha = R.ht_chart_fill_alpha
        tSet.setColor(R.ht_chart_color_blue_T)
        tSet.fillColor = R.ht_chart_color_blue_T
        tSet.circleColors = [R.ht_chart_color_blue_T]
        
        var dataSets = [LineChartDataSet]()
        dataSets.append(hSet)
        dataSets.append(tSet)
        
        //let data = LineChartData(xVals: times, dataSets: dataSets)
        let data = LineChartData(dataSets: dataSets)
        chartView.data = data
        
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

}
