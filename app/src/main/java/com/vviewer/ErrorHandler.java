package com.vviewer;

import android.os.Environment;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.io.StringWriter;

public class ErrorHandler implements Thread.UncaughtExceptionHandler {
    @Override
    public void uncaughtException(Thread thread, Throwable ex) {
        try {
            FileWriter fw = new FileWriter(Environment.getExternalStorageDirectory() + "/vviewer_crash.log");
            fw.write("V-Viewer FATAL ERROR\n");
            fw.write("Thread: " + thread.getName() + "\n");
            fw.write(ex.toString() + "\n");
            StringWriter sw = new StringWriter();
            ex.printStackTrace(new PrintWriter(sw));
            fw.write(sw.toString());
            fw.close();
        } catch (Exception ignored) {
        }
        // إنهاء التطبيق
        System.exit(1);
    }
}
