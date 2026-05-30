package com.vviewer;

import android.app.NativeActivity;
import android.content.Intent;
import android.os.Bundle;

public class MainActivity extends NativeActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    public void openSettings() {
        Intent intent = new Intent(this, SettingsActivity.class);
        startActivity(intent);
    }

    public void openTerminal() {
        Intent intent = new Intent(this, TerminalActivity.class);
        startActivity(intent);
    }
}
