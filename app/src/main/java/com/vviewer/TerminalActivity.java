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

        appendOutput("V-Viewer Terminal v1.0\n");
        appendOutput("=======================\n");

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

            appendOutput("Files dir: " + getFilesDir().getAbsolutePath() + "\n");

            // 1. التأكد من proot وتصحيح صلاحياته
            File prootFile = new File(prootPath);
            if (!prootFile.exists() || prootFile.length() == 0) {
                appendOutput("Installing proot...\n");
                InputStream in = getAssets().open("proot");
                FileOutputStream out = new FileOutputStream(prootFile);
                byte[] buf = new byte[8192];
                int len;
                while ((len = in.read(buf)) > 0) {
                    out.write(buf, 0, len);
                }
                in.close();
                out.close();
            }
            prootFile.setReadable(true);
            prootFile.setWritable(true);
            prootFile.setExecutable(true);
            appendOutput("proot ready (" + prootFile.length() + " bytes)\n");

            // 2. التأكد من وجود Ubuntu
            File ubuntuDir = new File(ubuntuPath);
            File bashFile = new File(ubuntuPath + "/bin/bash");
            
            if (!ubuntuDir.exists() || !bashFile.exists()) {
                appendOutput("Ubuntu not found at: " + ubuntuPath + "\n");
                appendOutput("bin/bash exists: " + bashFile.exists() + "\n");
                
                // محاولة فك الضغط إذا كانت الصورة موجودة
                File imageFile = new File(Environment.getExternalStorageDirectory() + "/V-Viewer/rootfs.tar.gz");
                if (imageFile.exists()) {
                    appendOutput("Image found (" + (imageFile.length()/1024/1024) + "MB). Extracting...\n");
                    appendOutput("This may take 5-15 minutes. Please wait...\n");
                    
                    ubuntuDir.mkdirs();
                    
                    ProcessBuilder pb = new ProcessBuilder(
                        prootPath,
                        "-r", ubuntuPath,
                        "/bin/tar", "-xzf", imageFile.getAbsolutePath(),
                        "-C", "/"
                    );
                    pb.redirectErrorStream(true);
                    Process process = pb.start();
                    BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
                    String line;
                    int count = 0;
                    while ((line = reader.readLine()) != null) {
                        // عرض تقدم كل 100 سطر
                        if (count % 100 == 0) {
                            appendOutput(".");
                        }
                        count++;
                    }
                    process.waitFor();
                    appendOutput("\nExtraction complete. Exit code: " + process.exitValue() + "\n");
                    
                    if (!bashFile.exists()) {
                        appendOutput("Error: Extraction failed. bash not found.\n");
                        appendOutput("Check if the image is valid.\n");
                        return;
                    }
                } else {
                    appendOutput("Error: No Ubuntu image found at " + imageFile.getAbsolutePath() + "\n");
                    appendOutput("Please install from Settings first.\n");
                    return;
                }
            } else {
                appendOutput("Ubuntu found at: " + ubuntuPath + "\n");
            }

            // 3. تشغيل الجلسة
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

            appendOutput("Ubuntu session started!\n");
            appendOutput("root@localhost:~# \n");
            
        } catch (Exception e) {
            appendOutput("Error: " + e.getMessage() + "\n");
            StringWriter sw = new StringWriter();
            e.printStackTrace(new PrintWriter(sw));
            appendOutput(sw.toString() + "\n");
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
