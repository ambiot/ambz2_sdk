//
//  DeviceCollectionViewCell.swift
//  WiGadget
//
//  Created by WU JINZHOU on 13/8/15.
//  Copyright (c) 2015 WU JINZHOU. All rights reserved.
//

import UIKit

class DeviceCollectionViewCell: UICollectionViewCell {
    @IBOutlet weak var findDeviceImageView: UIImageView!
    @IBOutlet weak var findDeviceTextLabel: UILabel!
    
    
    @IBOutlet weak var myDeviceImageView: UIImageView!
    @IBOutlet weak var myDeviceTextLabel: UILabel!
    
    var mac = R.not_assigned
    var image = R.not_assigned
}
