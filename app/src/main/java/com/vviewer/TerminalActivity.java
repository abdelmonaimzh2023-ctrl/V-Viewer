package com.vviewer;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;
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

        // بدء جلسة Ubuntu تلقائياً
        startUbuntuSession();

        sendBtn.setOnClickListener(v -> {
            String command = terminalInput.getText().toString();
            if (!command.isEmpty() && processInput != null) {
                try {
                    processInput.write((command + "\n").getBytes());
                    processInput.flush();
                    terminalInput.setText("");
                } catch (Exception e) {
                    appendOutput("Error: " + e.getMessage());
                }
            }
        });
    }

    private void startUbuntuSession() {
        try {
            String ubuntuPath = "/data/data/com.vviewer/files/ubuntu";
            String prootPath = getFilesDir() + "/proot";
            
            // التأكد من وجود proot
            File prootFile = new File(prootPath);
            if (!prootFile.exists()) {
                appendOutput("Error: proot binary not found. Please install first.");
                return;
            }
            prootFile.setExecutable(true);

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
                    appendOutput("Session ended.");
                }
            });
            outputThread.start();

            appendOutput("Ubuntu session started.\n");
        } catch (Exception e) {
            appendOutput("Failed to start Ubuntu: " + e.getMessage());
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
        if (ubuntuProcess != null) {
            ubuntuProcess.destroy();
        }
        if (outputThread != null) {
            outputThread.interrupt();
        }
    }
}
