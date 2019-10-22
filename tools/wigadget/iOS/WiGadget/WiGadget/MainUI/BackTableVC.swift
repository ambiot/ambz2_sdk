//
//  BackTableVC.swift
//  WiGadget
//
//  Created by WU JINZHOU on 23/7/15.
//  Copyright (c) 2015 WU JINZHOU. All rights reserved.
//

import UIKit



class BackTableVC: UITableViewController {
    
    @IBOutlet weak var deviceCntLabel: UILabel!
    
    @IBOutlet weak var deviceCntLabelMask: UIImageView!
    
    @IBOutlet weak var deviceFoundLabel: UILabel!
    
    @IBOutlet weak var deviceFoundLabelMask: UIImageView!
    
    
    override func viewDidLoad() {
        super.viewDidLoad()
        title = "Menu Page"
    }
    

    override func viewWillAppear(_ animated: Bool) {
        
        if AmebaList.savedList.count != 0 {
            deviceCntLabelMask.layer.cornerRadius = R.menu_label_corner_radius
            deviceCntLabelMask.layer.masksToBounds = true
            deviceCntLabelMask.isHidden = false
            
            deviceCntLabel.textAlignment = .center
            deviceCntLabel.text = "\(AmebaList.savedList.count)" + R.menu_label_trailing
            deviceCntLabel.isHidden = false
            
        }
        else{
            deviceCntLabel.isHidden = true
            deviceCntLabelMask.isHidden = true
        }
        
        if AmebaList.newfoundList.count != 0 {
            deviceFoundLabelMask.layer.cornerRadius = R.menu_label_corner_radius
            deviceFoundLabelMask.layer.masksToBounds = true
            deviceFoundLabelMask.isHidden = false
            
            deviceFoundLabel.textAlignment = .center
            deviceFoundLabel.text = "New \(AmebaList.newfoundList.count)" + R.menu_label_trailing
            deviceFoundLabel.isHidden = false
        }
        else{
            deviceFoundLabel.isHidden = true
            deviceFoundLabelMask.isHidden = true
        }
        
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
}
