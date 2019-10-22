//
//  RegisterFirebaseVC.swift
//  WiGadget
//
//  Created by WU JINZHOU on 19/9/15.
//  Copyright © 2015 WU JINZHOU. All rights reserved.
//

import UIKit

class RegisterFirebaseVC: UIViewController,UIWebViewDelegate {

    @IBOutlet weak var webView: UIWebView!
    
    var timer = Timer()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        webView.delegate = self
        let url = URL(string: "https://www.firebase.com/signup/")
        let request = URLRequest(url: url!)
        
        webView.loadRequest(request)
    }
    
    func cancelLoading() {
        timer.invalidate()
        view.hideToastActivity()
        view.makeToast("Connection timeout\nCannot open web page", duration: R.toast_duration, position: CSToastPositionBottom, title: "CONNECTION TIMEOUT", image: R.toast_ic_failed)
    }
    
    func webViewDidStartLoad(_ webView: UIWebView) {
        view.makeToastActivity()
        timer = Timer.scheduledTimer(timeInterval: R.web_page_load_timeout, target: self, selector: #selector(RegisterFirebaseVC.cancelLoading), userInfo: nil, repeats: false)
    }
    
    func webViewDidFinishLoad(_ webView: UIWebView) {
        timer.invalidate()
        view.hideToastActivity()
    }
    
    @IBAction func stop(_ sender: AnyObject) {
        webView.stopLoading()
    }
    
    
    @IBAction func reload(_ sender: AnyObject) {
        webView.reload()
    }
    
    
    @IBAction func goBack(_ sender: AnyObject) {
        webView.goBack()
    }
    
    
    @IBAction func goForward(_ sender: AnyObject) {
        webView.goForward()
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
    

    @IBAction func closeVC(_ sender: AnyObject) {
        timer.invalidate()
        self.dismiss(animated: true, completion: nil);
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
