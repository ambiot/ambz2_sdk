package com.realtek.wigadget;

import android.util.Log;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.net.InetAddress;
import java.net.Socket;

/**
 * Created by Wu Jinzhou on 4/28/2015.
 */
public class TcpClient {

    private static final Constants c = Constants.getInstance();

    public synchronized String echo(InetAddress serverAddress, int serverPort,String command)throws InterruptedException,IOException{

        Thread.sleep(c.val_tcp_next_cmd_waiting_ms);

        Log.v(c.tag_tcp_client,"echo("+serverAddress+","+serverPort+")"+" with command :"+command);

        Socket s = new Socket(serverAddress,serverPort);

        BufferedReader rx = new BufferedReader(new InputStreamReader(s.getInputStream()));

        PrintWriter tx = new PrintWriter(new BufferedWriter(new OutputStreamWriter(s.getOutputStream())),true);

        tx.println(command);

        Log.v(c.tag_tcp_client, "Tx :" + command);

        String response = rx.readLine();

        Log.v(c.tag_tcp_client,"Rx :"+response);

        s.close();

        return response;
    }

}
