package com.vviewer;

import android.app.NativeActivity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.widget.RelativeLayout;

public class MainActivity extends NativeActivity {
    private View splashView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // إظهار شاشة التحميل
        splashView = getLayoutInflater().inflate(R.layout.splash, null);
        addContentView(splashView, new RelativeLayout.LayoutParams(
            RelativeLayout.LayoutParams.MATCH_PARENT,
            RelativeLayout.LayoutParams.MATCH_PARENT));

        // إخفاء شاشة التحميل بعد 3 ثواني
        new Handler().postDelayed(() -> {
            if (splashView != null) {
                ((RelativeLayout)splashView.getParent()).removeView(splashView);
                splashView = null;
            }
        }, 3000);
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
