package com.vviewer;

import android.app.NativeActivity;
import android.os.Bundle;

public class MainActivity extends NativeActivity {
    static {
        // تفعيل معالج الأخطاء فور تحميل الكلاس
        Thread.setDefaultUncaughtExceptionHandler(new ErrorHandler());
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }
}
