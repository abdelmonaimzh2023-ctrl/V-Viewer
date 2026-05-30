package com.vviewer;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
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

        appendOutput("Initializing...\n");
        appendOutput("Files dir: " + getFilesDir().getAbsolutePath() + "\n");

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

            appendOutput("Looking for proot at: " + prootPath + "\n");

            File prootFile = new File(prootPath);
            if (!prootFile.exists()) {
                appendOutput("proot not found, copying from assets...\n");
                // نسخ من assets
                InputStream in = getAssets().open("proot");
                FileOutputStream out = new FileOutputStream(prootFile);
                byte[] buf = new byte[8192];
                int len;
                while ((len = in.read(buf)) > 0) {
                    out.write(buf, 0, len);
                }
                in.close();
                out.close();
                prootFile.setExecutable(true);
                appendOutput("proot copied (" + prootFile.length() + " bytes)\n");
            }

            if (!prootFile.exists() || prootFile.length() == 0) {
                appendOutput("Error: proot binary is missing or empty.\n");
                return;
            }

            // التحقق من وجود Ubuntu
            File ubuntuDir = new File(ubuntuPath);
            if (!ubuntuDir.exists() || !new File(ubuntuPath + "/bin/bash").exists()) {
                appendOutput("Error: Ubuntu not installed. Please install the image first.\n");
                appendOutput("Looked in: " + ubuntuPath + "\n");
                appendOutput("bin/bash exists: " + new File(ubuntuPath + "/bin/bash").exists() + "\n");
                return;
            }

            appendOutput("Starting Ubuntu session...\n");

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

            appendOutput("Ubuntu session started.\n");
        } catch (Exception e) {
            appendOutput("Failed to start Ubuntu: " + e.getMessage() + "\n");
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
