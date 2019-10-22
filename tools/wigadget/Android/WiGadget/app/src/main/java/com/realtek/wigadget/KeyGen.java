package com.realtek.wigadget;

import android.util.Log;

import org.apaches.commons.codec.digest.DigestUtils;

import java.util.Arrays;
import java.util.Random;

import djb.Curve25519;

/**
 * Created by Wu Jinzhou on 4/27/2015.
 */
public class KeyGen {
    private static KeyGen thisKeyGen;
    private KeyGen(){}
    public static synchronized KeyGen getInstance(){
        if(thisKeyGen==null){
            thisKeyGen = new KeyGen();
        }
        return thisKeyGen;
    }

    private static final Constants c = Constants.getInstance();
    private byte[] a_private_key_byte = new byte[32];
    private  byte[] a_public_key_byte =new byte[32];
    private  byte[] b_public_key_byte =new byte[32];
    private  byte[] shared_key_byte = new byte[32];
    private byte[] aes_128_key = new byte[16]; //AES 128 bit encryption
    private short[] a_public_key_short = new short[32];

    public synchronized void makeKeys (String keyB){

        String seed= ((Double)(new Random(System.currentTimeMillis())).nextDouble()).toString();
        a_private_key_byte = DigestUtils.md5Hex(seed).getBytes();

        a_public_key_byte =new byte[]{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

        Curve25519.keygen(a_public_key_byte,null, a_private_key_byte);

        String[] items = keyB.replaceAll("\\[", "").replaceAll("\\]", "").replaceAll("\\ ", "").split(",");
        //String[] items = keyB.split(",");

        int[] results = new int[items.length];

        for (int i = 0; i < items.length; i++) {
            try {
                results[i] = Integer.parseInt(items[i]);
            } catch (NumberFormatException e) {
                Log.e(c.tag_key_gen,"makeKeys() (NumberFormatException e "+e);
            }
        }

        for (int i =0;i<results.length;i++){
            b_public_key_byte[i]=(byte)results[i];
        }

        Curve25519.curve(shared_key_byte, a_private_key_byte, b_public_key_byte);

        a_public_key_short=toShort(a_public_key_byte);

        aes_128_key=Arrays.copyOf(shared_key_byte,16);

        Log.v(c.tag_key_gen,"makeKeys() final AES key: "+getAES128KeyString());
    }

    public synchronized String getPublicKeyString(){
        return Arrays.toString(a_public_key_short);
    }

    public synchronized String getAES128KeyString(){
        return Arrays.toString(aes_128_key);
    }

    private synchronized short[] toShort (byte[] b){
        short s [] = new short[b.length];
        for (int i=0;i<b.length;i++){
            s[i]=(short)(b[i]&0xff);
        }
        return s;
    }
}
