package com.realtek.wigadget;

import java.util.Arrays;

import javax.crypto.Cipher;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;

/**
 * Created by Wu Jinzhou on 5/6/2015.
 */
public class AES128 {

    private static final String IV = "AAAAAAAAAAAAAAAA";
    private static final int byteUnit =16;

    private synchronized byte[] encrypt(String plainText, byte[] encryptionKey) throws Exception {
        //padding
        int boz=plainText.length()%byteUnit;
        if(boz!=0){
            for (int i=0 ; i<(byteUnit-boz); i++){
                plainText+='\0';
            }
        }
        //Cipher cipher = Cipher.getInstance("AES/CBC/NoPadding", "SunJCE");
        Cipher cipher = Cipher.getInstance("AES/CBC/NoPadding");
        SecretKeySpec key = new SecretKeySpec(encryptionKey, "AES");
        cipher.init(Cipher.ENCRYPT_MODE, key,new IvParameterSpec(IV.getBytes("UTF-8")));
        return cipher.doFinal(plainText.getBytes("UTF-8"));
    }

    private synchronized String decrypt(byte[] cipherText, byte[] encryptionKey) throws Exception{
        //Cipher cipher = Cipher.getInstance("AES/CBC/NoPadding", "SunJCE");
        Cipher cipher = Cipher.getInstance("AES/CBC/NoPadding");
        SecretKeySpec key = new SecretKeySpec(encryptionKey, "AES");
        cipher.init(Cipher.DECRYPT_MODE, key,new IvParameterSpec(IV.getBytes("UTF-8")));
        return new String(cipher.doFinal(cipherText),"UTF-8");
    }

    private synchronized byte[] parseByteArrayFromString(String stringOfByteArray){

        //parse format: "[ -1, 2, -3, 4, -5]"

        String[] items = stringOfByteArray.replaceAll("\\[", "").replaceAll("\\]", "").replaceAll("\\ ","").split(",");

        int[] resultsInInt = new int[items.length];

        byte[] resultsInByte = new byte[items.length];

        for (int i = 0; i < items.length; i++) {
            resultsInInt[i] = Integer.parseInt(items[i]);
        }

        for (int i =0;i<resultsInInt.length;i++){
            resultsInByte[i]=(byte)resultsInInt[i];
        }

        return resultsInByte;
    }

    public synchronized String encryptToStringOfShortArray(String plainText,String keyArrayInString)throws Exception{
        byte [] key = parseByteArrayFromString(keyArrayInString);
        byte[] encryptedDataInByte = encrypt(plainText, key);
        short[] encryptedDataInShort = new short[encryptedDataInByte.length];
        for (int i=0; i<encryptedDataInByte.length;i++){
            encryptedDataInShort[i]=(short)(encryptedDataInByte[i]&0xff);
        }
        return Arrays.toString(encryptedDataInShort);
    }

    public synchronized String decryptFromStringOfShortArray(String cipherTextArrayInString, String keyArrayInString)throws Exception{
        byte[] cipherText = parseByteArrayFromString(cipherTextArrayInString);
        byte[] key = parseByteArrayFromString(keyArrayInString);
        return decrypt(cipherText,key);
    }
}
