package com.realtek.wigadget;

import android.app.Activity;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Handler;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Toast;

/**
 * Created by Wu Jinzhou on 5/11/2015.
 */
public class RegisterFirebaseAccountActivity extends Activity {
    private static final Constants c = Constants.getInstance();
    WebView web;
    boolean exitActivity =false;
    boolean showToast = true;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_register_firebase_account);

        web = (WebView) findViewById(R.id.webView_register_firebase_account);
        web.setWebViewClient(new myWebClient());
        web.getSettings().setJavaScriptEnabled(true);
        web.loadUrl(c.val_firebase_register_url);
    }

    public class myWebClient extends WebViewClient
    {
        @Override
        public void onPageStarted(WebView view, String url, Bitmap favicon) {

            super.onPageStarted(view, url, favicon);
        }

        @Override
        public boolean shouldOverrideUrlLoading(WebView view, String url) {

            view.loadUrl(url);
            return true;

        }
    }


// To handle "Back" key press event for WebView to go back to previous screen.

    @Override
    public void onBackPressed() {

        if(showToast){
            Toast.makeText(this, "Nav Options:\nSingle-click: Back\nDouble-Click: Quit", Toast.LENGTH_LONG).show();
            showToast =false;
        }
        if (exitActivity) {
            super.onBackPressed();
        }
        else{
            if(web.canGoBack()){
                web.goBack();
            }
        }

        exitActivity = true;
        new Handler().postDelayed(new Runnable() {

            @Override
            public void run() {
                exitActivity =false;
            }
        }, c.val_ui_double_click_interval);
    }
}
