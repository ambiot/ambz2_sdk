//
//  HTSettingsVC.swift
//  WiGadget
//
//  Created by WU JINZHOU on 14/9/15.
//  Copyright (c) 2015 WU JINZHOU. All rights reserved.
//

import UIKit
import AVFoundation

class HTSettingsVC: UITableViewController, UITextFieldDelegate {
    
    //section 0 cell 0
    @IBOutlet weak var s0c0SB: UISwitch!
    
    //section 0 cell 1
    @IBOutlet weak var s0c1IV: UIImageView!
    @IBOutlet weak var s0c1TL: UILabel!
    
    //section 0 cell 2
    @IBOutlet weak var s0c2IV: UIImageView!
    @IBOutlet weak var s0c2TL: UILabel!
    
    //section 0 cell 3
    @IBOutlet weak var s0c3IV: UIImageView!
    @IBOutlet weak var s0c3TL: UILabel!
    
    
    //section 0 cell 4 -> humi
    @IBOutlet weak var s0c4TF: UITextField!
    @IBOutlet weak var s0c4TL: UILabel!
    
    //section 0 cell 5 -> temp
    @IBOutlet weak var s0c5TF: UITextField!
    @IBOutlet weak var s0c5TL: UILabel!
    
    //section 1 cell 0
    @IBOutlet weak var s1c0IV: UIImageView!
    
    //section 1 cell 1
    @IBOutlet weak var s1c1IV: UIImageView!
    
    //section 1 cell 2
    @IBOutlet weak var s1c2IV: UIImageView!
    
    var cellHeight = R.settings_tv_cell_height
    
    var s0IV = [UIImageView]()
    var s0TL = [UILabel]()
    var s0TF = [UITextField]()
    var s1IV = [UIImageView]()
    
    var useAlarm = false
    var alarmSound = 0 //mapped to sound 1
    var humiThres = 0.0
    var tempThres = 0.0
    var linkFreq = 2
    
    var player = AVAudioPlayer()
    var canPlaySound = false
    
    
    /*
    Tells the delegate that the table view is about to draw a cell for a particular row.
    */
    override func tableView(_ tableView: UITableView, willDisplay cell: UITableViewCell,
        forRowAt indexPath: IndexPath)
    {
        // Remove separator inset
        if cell.responds(to: #selector(setter: UITableViewCell.separatorInset)) {
            cell.separatorInset = UIEdgeInsets.zero
        }
        
        // Prevent the cell from inheriting the Table View's margin settings
        if cell.responds(to: #selector(setter: UIView.preservesSuperviewLayoutMargins)) {
            cell.preservesSuperviewLayoutMargins = false
        }
        
        // Explictly set your cell's layout margins
        if cell.responds(to: #selector(setter: UIView.layoutMargins)) {
            cell.layoutMargins = UIEdgeInsets.zero
        }
    }
    

    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = "H&T Sensor"
        
        s0TL = [s0c1TL,s0c2TL,s0c3TL,s0c4TL,s0c5TL]
        s0IV = [s0c1IV,s0c2IV,s0c3IV]
        s0TF = [s0c4TF,s0c5TF]
        s1IV = [s1c0IV,s1c1IV,s1c2IV]
        
        
        s0c0SB.addTarget(self, action: #selector(HTSettingsVC.s0c0SBToggled(_:)), for: UIControlEvents.valueChanged)
        
        s0c4TF.delegate = self
        s0c5TF.delegate = self
        
        loadHTSettings()
        
        initUI()
        
    }
    
    //update & save on TF changed
    func textFieldShouldReturn(_ textField: UITextField) -> Bool {
        
        humiThres = (s0c4TF.text! as NSString).doubleValue
        Log.i("humiThres: \(humiThres)")
        
        tempThres = (s0c5TF.text! as NSString).doubleValue
        Log.i("tempThres: \(tempThres)")
       
        view.endEditing(true)
        saveHTSettings()
        
        return true
    }
    
    override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
        
        if indexPath.section == 0 {
            if indexPath.row == 0 {
                return R.settings_tv_cell_height
            }
            return  cellHeight
        }
        else {
            return R.settings_tv_cell_height
        }
    }
    
    //update & save on cell choosed
    override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        let section = indexPath.section
        let row = indexPath.row
        Log.i("selected cell @ section: \(section) row: \(row)")
        
        if rowToData(section: section, row: row) {
            cellSelected(section: section, row: row)
            saveHTSettings()
        }
        
    }
    
    func cellSelected(section:Int, row:Int) {
        if section == 0 {
            if 1 ... 3 ~= row {
                for i in 0 ..< s0IV.count {
                    s0IV[i].image = nil
                }
                s0IV[row - 1].image = UIImage(named: "ic_selected_55x55")
                if canPlaySound {
                    playSound(row - 1)
                }
            }
        }
        if section == 1 {
            for i in 0 ..< s1IV.count {
                s1IV[i].image = nil
            }
            s1IV[row].image = UIImage(named: "ic_selected_55x55")
        }
        tableView.reloadData()
    }
    
    //update & save on SB toggled
    func s0c0SBToggled(_ switchState: UISwitch) {
        if switchState.isOn {
            Log.i("s0c0SBT on")
            useAlarm = true
            s0Hide(false)
        } else {
            Log.i("s0c0SBT off")
            useAlarm = false
            s0Hide(true)
        }
        saveHTSettings()
    }
    
    func s0Hide(_ hidden:Bool) {
        
        s0c0SB.isOn = !hidden
        
        for i in 0 ..< s0IV.count {
            s0IV[i].isHidden = hidden
        }
        
        for i in 0 ..< s0TF.count {
            s0TF[i].isHidden = hidden
        }
        
        for i in 0 ..< s0TL.count {
            s0TL[i].isHidden = hidden
        }
        
        if hidden {
            cellHeight = 0
        }
        else {
            cellHeight = R.settings_tv_cell_height
        }
        
        tableView.reloadData()
    }
    
    
    func saveHTSettings() {
        
        let htSettings = [R.settings_key_ht_enable_alarm:useAlarm,
                          R.settings_key_ht_alarm_sound:alarmSound,
                          R.settings_key_ht_humi_thres:humiThres,
                          R.settings_key_ht_temp_thres:tempThres,
                          R.settings_key_ht_link_freq:linkFreq] as [String : Any]
        
        let data_to_save = JSON(htSettings).rawString(String.Encoding.utf8, options: [])
        let defaults = UserDefaults.standard
        defaults.setValue(data_to_save, forKey: R.settings_key_ht)
        defaults.synchronize()
    }
    
    
    func loadHTSettings() {
        let defaults = UserDefaults.standard
        let dataStr = defaults.value(forKey: R.settings_key_ht) as? String ?? R.not_assigned
        let htSettings = JSON(data: dataStr.data(using: String.Encoding.utf8, allowLossyConversion: false)!)
        
        if !(htSettings.isEmpty) {
            useAlarm = htSettings[R.settings_key_ht_enable_alarm].boolValue
            alarmSound = htSettings[R.settings_key_ht_alarm_sound].intValue
            humiThres = htSettings[R.settings_key_ht_humi_thres].doubleValue
            tempThres = htSettings[R.settings_key_ht_temp_thres].doubleValue
            linkFreq = htSettings[R.settings_key_ht_link_freq].intValue
        }
    }
    
    
    func initUI() {
        if !useAlarm {
            s0Hide(true)
        }
        else {
            s0Hide(false)
        }
        cellSelected(section: 0, row: dadaToRow(section: 0, dataVal: alarmSound)!)
        s0c4TF.text = "\(humiThres)"
        s0c5TF.text = "\(tempThres)"
        cellSelected(section: 1, row: dadaToRow(section: 1, dataVal: Int(linkFreq))!)
        canPlaySound = true
    }
    
    func dadaToRow (section:Int, dataVal:Int) -> Int? {
        if section == 0 {
            let row = dataVal + 1
            if row < 1 {
                return 1
            }
            if row > 3 {
                return 3
            }
            return row
        }
        if section == 1 {
            let row = (dataVal / 2) - 1
            if row < 0 {
                return 0
            }
            if row > 2 {
                return 2
            }
            return row
        }
        return nil
    }
    
    
    func rowToData (section:Int , row:Int) -> Bool {
        if section == 0 {
            if row < 1 {
                return false
            }
            if row > 3 {
                return false
            }
            alarmSound = row - 1
            return true
        }
        if section == 1 {
            if row < 0 {
                return false
            }
            if row > 2 {
                return false
            }
            linkFreq = (row + 1) * 2
            return true
        }
        return false
    }
    
    func playSound (_ idx:Int) {
        if 0 ..< R.settings_ht_alarm_sounds.count ~= idx {
            
            let path = Bundle.main.path(forResource: R.settings_ht_alarm_sounds[idx], ofType: "wav")
            
            try! player = AVAudioPlayer(contentsOf: URL(fileURLWithPath: path!))
    
            player.prepareToPlay()
            player.play()
        }
    }
    
    
    
    @IBAction func doneEditing(_ sender: AnyObject) {
        self.dismiss(animated: true, completion: nil);
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }


}
