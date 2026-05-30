package com.vviewer;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.TextView;
import java.io.*;

public class TerminalActivity extends Activity {
    private TextView terminalOutput;
    private EditText terminalInput;
    private ScrollView scrollView;
    private Process ubuntuProcess;
    private OutputStream processInput;
    private InputStream processOutput;
    private Thread outputThread;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.terminal);

        terminalOutput = findViewById(R.id.terminalOutput);
        terminalInput = findViewById(R.id.terminalInput);
        scrollView = findViewById(R.id.scrollView);
        Button sendBtn = findViewById(R.id.sendBtn);

        appendOutput("V-Viewer Terminal v1.0\n");
        startUbuntuSession();

        sendBtn.setOnClickListener(v -> {
            String command = terminalInput.getText().toString();
            if (!command.isEmpty() && processInput != null) {
                try {
                    processInput.write((command + "\n").getBytes());
                    processInput.flush();
                    terminalInput.setText("");
                } catch (Exception e) {
                    appendOutput("Error: " + e.getMessage() + "\n");
                }
            }
        });
    }

    private void startUbuntuSession() {
        try {
            String ubuntuPath = getFilesDir() + "/ubuntu";
            String prootPath = getFilesDir() + "/proot";

            // نسخ proot إذا لم يكن موجوداً
            File prootFile = new File(prootPath);
            if (!prootFile.exists() || prootFile.length() == 0) {
                appendOutput("Copying proot...\n");
                InputStream in = getAssets().open("proot");
                FileOutputStream out = new FileOutputStream(prootFile);
                byte[] buf = new byte[8192];
                int len;
                while ((len = in.read(buf)) > 0) out.write(buf, 0, len);
                in.close(); out.close();
            }

            // تصحيح الصلاحيات (تنفيذ + قراءة + كتابة)
            prootFile.setExecutable(true, false);
            prootFile.setReadable(true, false);
            prootFile.setWritable(true, false);

            if (!prootFile.canExecute()) {
                appendOutput("Error: Cannot execute proot\n");
                return;
            }

            // التأكد من وجود Ubuntu
            File ubuntuDir = new File(ubuntuPath);
            if (!ubuntuDir.exists() || !new File(ubuntuPath + "/bin").exists()) {
                appendOutput("Ubuntu not installed. Please use Settings first.\n");
                return;
            }

            appendOutput("Starting Ubuntu...\n");
            ProcessBuilder pb = new ProcessBuilder(
                prootPath,
                "-r", ubuntuPath,
                "-b", "/dev",
                "-b", "/proc",
                "-b", "/sys",
                "/bin/bash"
            );
            pb.redirectErrorStream(true);
            ubuntuProcess = pb.start();
            processInput = ubuntuProcess.getOutputStream();
            processOutput = ubuntuProcess.getInputStream();

            outputThread = new Thread(() -> {
                byte[] buffer = new byte[1024];
                int len;
                try {
                    while ((len = processOutput.read(buffer)) > 0) {
                        String text = new String(buffer, 0, len);
                        appendOutput(text);
                    }
                } catch (IOException e) {
                    appendOutput("Session ended.\n");
                }
            });
            outputThread.start();
            appendOutput("Ubuntu started!\n");
        } catch (Exception e) {
            appendOutput("Error: " + e.getMessage() + "\n");
        }
    }

    private void appendOutput(String text) {
        runOnUiThread(() -> {
            terminalOutput.append(text);
            scrollView.post(() -> scrollView.fullScroll(View.FOCUS_DOWN));
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (ubuntuProcess != null) ubuntuProcess.destroy();
        if (outputThread != null) outputThread.interrupt();
    }
}
