package com.vviewer;

import android.app.NativeActivity;
import android.content.Intent;
import android.os.Bundle;

public class MainActivity extends NativeActivity {
    public void openSettings() {
        Intent intent = new Intent(this, SettingsActivity.class);
        startActivity(intent);
    }
}
