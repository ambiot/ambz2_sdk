//
//  AboutVC.swift
//  WiGadget
//
//  Created by WU JINZHOU on 24/7/15.
//  Copyright (c) 2015 WU JINZHOU. All rights reserved.
//

import UIKit

class AboutVC: UIViewController {

    @IBOutlet weak var menuBtn: UIBarButtonItem!
    
    @IBOutlet weak var dateLabel: UILabel!
    
    override func viewDidLoad() {
        super.viewDidLoad()
       
        menuBtn.target = self.revealViewController()
        menuBtn.action = #selector(SWRevealViewController.revealToggle(_:))
        self.view.addGestureRecognizer(self.revealViewController().panGestureRecognizer())
        self.title = R.menu_titles[3]
        
        dateLabel.text = R.app_build_date
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
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
