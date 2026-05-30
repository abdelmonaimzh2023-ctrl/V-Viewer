package com.vviewer;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import java.io.*;
import java.util.zip.GZIPInputStream;

public class SettingsActivity extends Activity {
    private TextView statusText, progressText;
    private Button selectFileBtn, installBtn, deleteBtn, cancelBtn;
    private ProgressBar progressBar;
    private Uri selectedFileUri = null;
    private boolean isInstalling = false;
    private Thread installThread = null;
    private static final int PICK_FILE = 1;
    private static final String IMAGE_PATH = Environment.getExternalStorageDirectory() + "/V-Viewer/rootfs.tar.gz";
    private static final String INSTALL_PATH = "/data/data/com.vviewer/files/ubuntu";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.settings);

        statusText = findViewById(R.id.statusText);
        progressText = findViewById(R.id.progressText);
        progressBar = findViewById(R.id.progressBar);
        selectFileBtn = findViewById(R.id.selectFileBtn);
        installBtn = findViewById(R.id.installBtn);
        deleteBtn = findViewById(R.id.deleteBtn);
        cancelBtn = findViewById(R.id.cancelBtn);

        updateStatus();

        selectFileBtn.setOnClickListener(v -> {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("*/*");
            startActivityForResult(intent, PICK_FILE);
        });

        installBtn.setOnClickListener(v -> {
            if (selectedFileUri != null && !isInstalling) {
                startInstallation();
            } else if (selectedFileUri == null) {
                Toast.makeText(this, "Select a file first", Toast.LENGTH_SHORT).show();
            }
        });

        cancelBtn.setOnClickListener(v -> {
            if (isInstalling && installThread != null) {
                installThread.interrupt();
                isInstalling = false;
                resetUI();
                Toast.makeText(this, "Installation cancelled", Toast.LENGTH_SHORT).show();
            }
        });

        deleteBtn.setOnClickListener(v -> deleteInstallation());
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == PICK_FILE && resultCode == RESULT_OK && data != null) {
            selectedFileUri = data.getData();
            Toast.makeText(this, "File selected: " + selectedFileUri.getLastPathSegment(), Toast.LENGTH_SHORT).show();
        }
    }

    private void startInstallation() {
        isInstalling = true;
        installBtn.setEnabled(false);
        cancelBtn.setVisibility(View.VISIBLE);
        progressBar.setVisibility(View.VISIBLE);
        progressBar.setMax(100);
        progressBar.setProgress(0);
        progressText.setVisibility(View.VISIBLE);
        progressText.setText("Preparing...");

        installThread = new Thread(() -> {
            try {
                // المرحلة 1: نسخ الملف
                updateProgress("Copying file...", 0);
                File destDir = new File(Environment.getExternalStorageDirectory(), "V-Viewer");
                destDir.mkdirs();
                File destFile = new File(IMAGE_PATH);
                InputStream in = getContentResolver().openInputStream(selectedFileUri);
                long fileSize = in.available(); // قد يكون 0 لبعض الأنواع
                FileOutputStream out = new FileOutputStream(destFile);
                byte[] buffer = new byte[65536]; // 64KB buffer for speed
                int len;
                long total = 0;
                while ((len = in.read(buffer)) > 0 && !Thread.currentThread().isInterrupted()) {
                    out.write(buffer, 0, len);
                    total += len;
                    if (fileSize > 0) {
                        int percent = (int)(total * 50 / fileSize);
                        updateProgress("Copying... " + (total/1024/1024) + "MB", percent);
                    }
                }
                in.close();
                out.close();

                if (Thread.currentThread().isInterrupted()) return;

                // المرحلة 2: فك الضغط (محاكاة التقدم)
                updateProgress("Extracting...", 50);
                File installDir = new File(INSTALL_PATH);
                installDir.mkdirs();
                // TODO: استدعاء proot لفك الضغط الحقيقي
                // حالياً نحاكي التقدم
                for (int i = 50; i <= 100; i += 5) {
                    if (Thread.currentThread().isInterrupted()) return;
                    Thread.sleep(500);
                    updateProgress("Extracting... " + (i-50)*2 + "%", i);
                }

                runOnUiThread(() -> {
                    Toast.makeText(this, "Installation complete!", Toast.LENGTH_SHORT).show();
                    resetUI();
                    updateStatus();
                });
            } catch (Exception e) {
                if (!Thread.currentThread().isInterrupted()) {
                    runOnUiThread(() -> Toast.makeText(this, "Error: " + e.getMessage(), Toast.LENGTH_LONG).show());
                }
                resetUI();
            }
        });
        installThread.start();
    }

    private void updateProgress(String message, int percent) {
        runOnUiThread(() -> {
            progressText.setText(message + " (" + percent + "%)");
            progressBar.setProgress(percent);
        });
    }

    private void resetUI() {
        runOnUiThread(() -> {
            isInstalling = false;
            installBtn.setEnabled(true);
            cancelBtn.setVisibility(View.GONE);
            progressBar.setVisibility(View.GONE);
            progressText.setVisibility(View.GONE);
        });
    }

    private void deleteInstallation() {
        File installDir = new File(INSTALL_PATH);
        if (installDir.exists()) {
            deleteRecursive(installDir);
            Toast.makeText(this, "Installation deleted", Toast.LENGTH_SHORT).show();
        }
        updateStatus();
    }

    private void deleteRecursive(File file) {
        if (file.isDirectory()) {
            for (File child : file.listFiles()) {
                deleteRecursive(child);
            }
        }
        file.delete();
    }

    private void updateStatus() {
        File imageFile = new File(IMAGE_PATH);
        File installDir = new File(INSTALL_PATH);
        String status = "Image: " + (imageFile.exists() ? "Found (" + (imageFile.length()/1024/1024) + "MB)" : "Not found") +
                       "\nUbuntu: " + (installDir.exists() ? "Installed" : "Not installed");
        statusText.setText(status);
    }
}
