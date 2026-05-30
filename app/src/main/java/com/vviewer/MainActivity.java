package com.vviewer;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import java.io.FileWriter;

public class MainActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        TextView tv = new TextView(this);
        tv.setText("Loading...");
        setContentView(tv);
        
        try {
            System.loadLibrary("vviewer");
            writeResult("SUCCESS: libvviewer.so loaded");
            tv.setText("SUCCESS");
        } catch (UnsatisfiedLinkError e) {
            writeResult("ERROR: " + e.toString());
            tv.setText("ERROR: " + e.getMessage());
        }
    }

    private void writeResult(String msg) {
        try {
            FileWriter fw = new FileWriter("/sdcard/vviewer_load.log");
            fw.write(msg);
            fw.close();
        } catch (Exception ignored) {}
    }
}
