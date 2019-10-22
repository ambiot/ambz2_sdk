//
//  SettingsVC.swift
//  WiGadget
//
//  Created by WU JINZHOU on 24/7/15.
//  Copyright (c) 2015 WU JINZHOU. All rights reserved.
//

import UIKit

class SettingsVC: UITableViewController {

    @IBOutlet weak var menuBtn: UIBarButtonItem!
   
    
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
        menuBtn.target = self.revealViewController()
        menuBtn.action = #selector(SWRevealViewController.revealToggle(_:))
        self.view.addGestureRecognizer(self.revealViewController().panGestureRecognizer())
        self.title = R.menu_titles[2]
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
