//
//  Crypto.swift
//  WiGadget
//
//  Created by WU JINZHOU on 27/8/15.
//  Copyright (c) 2015 WU JINZHOU. All rights reserved.
//

import Foundation

class Crypto {
    
    static var curve25519_private_key = [UInt8]()
    static var curve25519_base_point = R.curve25519_base_point
    static var curve25519_my_public_key = [UInt8](repeating: 0, count: 32)
    static var curve25519_his_public_key = [UInt8]()
    static var curve25519_shared_key = [UInt8](repeating: 0, count: 32)
    static var aes_128_key = [UInt8]()
    static var aes_iv = [UInt8](R.aes_iv.utf8)
    static var aes_block_size = aes_iv.count
    
    //generate curve25519 public key
    class func makeCurve25519PublicKey() -> String {
        
        for _ in 0 ..< 32 {
            curve25519_private_key.append(UInt8(arc4random() % 256))
        }
        
        curve25519_donna(&curve25519_my_public_key, &curve25519_private_key, &curve25519_base_point)
        
        return "\(curve25519_my_public_key)"
    }
    
    //generate curve25519 shared key & AES shared key
    class func makePSK(_ hisCurve25519PublicKey:String) -> String {
        
        let strArr = hisCurve25519PublicKey.characters.split { $0 == "," }.map { String($0) }
        
        for item in strArr {
            let components = item.components(separatedBy: CharacterSet.decimalDigits.inverted)
            let part = components.joined(separator: "")
            if let intVal = Int(part) {
                curve25519_his_public_key.append(UInt8(intVal))
            }
        }
        
        curve25519_donna(&curve25519_shared_key, &curve25519_private_key, &curve25519_his_public_key)
        
        aes_128_key = [UInt8](curve25519_shared_key[0...15])
        
        return "\(aes_128_key)"
    }
    
    //aes128 encryption
    class func encrypt(_ plainText:String,key:String) -> String {
        
        var k8 = [UInt8]()
        
        let strArr = key.characters.split { $0 == "," }.map { String($0) }
        
        for item in strArr {
            let components = item.components(separatedBy: CharacterSet.decimalDigits.inverted)
            let part = components.joined(separator: "")
            if let intVal = Int(part) {
                k8.append(UInt8(intVal))
            }
        }
        
        var pt8 = [UInt8](plainText.utf8)
        
        //zero padding
        let r = pt8.count % aes_block_size
        
        for _ in 0 ..< (aes_block_size - r) {
            pt8.append(0)
        }
        
        var ct8 = pt8 //allocate mem for ct8
        
        AES128_CBC_encrypt_buffer(&ct8, &pt8, UInt32(pt8.count), &k8, &aes_iv)
        
        return "\(ct8)"
    }
    
    class func decrypt(_ cipherText:String,key:String) -> String {
        
        var k8 = [UInt8]()
        
        var strArr = key.characters.split { $0 == "," }.map { String($0) }
        
        for item in strArr {
            let components = item.components(separatedBy: CharacterSet.decimalDigits.inverted)
            let part = components.joined(separator: "")
            if let intVal = Int(part) {
                k8.append(UInt8(intVal))
            }
        }
        
        var ct8 = [UInt8]()
        
        strArr = cipherText.characters.split { $0 == "," }.map { String($0) }
        
        for item in strArr {
            let components = item.components(separatedBy: CharacterSet.decimalDigits.inverted)
            let part = components.joined(separator: "")
            if let intVal = Int(part) {
                ct8.append(UInt8(intVal))
            }
        }
        
        var pt8 = ct8 //allocate mem for pt8
        
        AES128_CBC_decrypt_buffer(&pt8, &ct8, UInt32(ct8.count), &k8, &aes_iv)
        
        pt8 = pt8.filter({$0 != 0})
        
        let pt = NSString(bytes: pt8, length: pt8.count, encoding: String.Encoding.utf8.rawValue) as! String
        
        return pt
    }
}
