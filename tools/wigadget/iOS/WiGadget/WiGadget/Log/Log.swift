//
//  Log.swift
//  bonjour
//
//  Created by WU JINZHOU on 12/8/15.
//  Copyright (c) 2015 WU JINZHOU. All rights reserved.
//


// To print color on console, install XcodeColors plugin for xCode
// https://github.com/robbiehanson/XcodeColors

import Foundation

class Log {
    
    static var ESCAPE = ""
    static var RESET_FG = ""
    static var RESET_BG = ""
    static var RESET = ""
    static var RED = ""
    static var BLUE = ""
    static var GREEN = ""
    
    class func e<T>(_ object:T, functionName:  String = #function, fileNameWithPath: String = #file, lineNumber: Int = #line ) {
        if R.debug_message_enable {
            
            setColor()
            
            let fileNameWithoutPath = URL(fileURLWithPath: fileNameWithPath).lastPathComponent
            let caller = "\(Date()): [\(functionName) in \(fileNameWithoutPath), line \(lineNumber)]: "
            
            print("\(ESCAPE)\(RED)\(caller)\(RESET)")
            print("\(ESCAPE)\(RED)\(object)\(RESET)")
            print("")
        }
    }
    
    class func i<T>(_ object:T) {
        if R.debug_message_enable {
            
            setColor()
            
            print("\(ESCAPE)\(GREEN)\(object)\(RESET)")
            print("")
        }
    }
    
    class func v<T>(_ object:T, functionName:  String = #function, fileNameWithPath: String = #file, lineNumber: Int = #line ) {
        if R.debug_message_enable {
            
            setColor()
            
            let fileNameWithoutPath = URL(fileURLWithPath: fileNameWithPath).lastPathComponent
            let caller = "\(Date()): [\(functionName) in \(fileNameWithoutPath), line \(lineNumber)]: "
            
            print("\(ESCAPE)\(BLUE)\(caller)\(RESET)")
            print("\(ESCAPE)\(BLUE)\(object)\(RESET)")
            print("")
        }
    }
    
    class func setColor(){
        if R.debug_message_in_color {
            ESCAPE = "\u{001b}["
            RESET_FG = ESCAPE + "fg;" // Clear any foreground color
            RESET_BG = ESCAPE + "bg;" // Clear any background color
            RESET = ESCAPE + ";"   // Clear any foreground or background color
            
            RED = "fg255,0,0;"
            GREEN = "fg0,131,18;"
            BLUE = "fg41,52,212;"
        }
    }
}
