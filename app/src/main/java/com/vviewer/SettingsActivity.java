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
    private static final String PROOT_PATH = "/data/data/com.vviewer/files/proot";

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

        copyProotFromAssets();
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

    private void copyProotFromAssets() {
        File prootFile = new File(PROOT_PATH);
        if (!prootFile.exists()) {
            try {
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
                Toast.makeText(this, "proot copied to " + PROOT_PATH, Toast.LENGTH_SHORT).show();
            } catch (Exception e) {
                Toast.makeText(this, "Failed to copy proot: " + e.getMessage(), Toast.LENGTH_LONG).show();
            }
        }
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
                updateProgress("Copying image...", 0);
                File destDir = new File(Environment.getExternalStorageDirectory(), "V-Viewer");
                destDir.mkdirs();
                File destFile = new File(IMAGE_PATH);
                InputStream in = getContentResolver().openInputStream(selectedFileUri);
                FileOutputStream out = new FileOutputStream(destFile);
                byte[] buffer = new byte[65536];
                int len;
                long total = 0;
                while ((len = in.read(buffer)) > 0 && !Thread.currentThread().isInterrupted()) {
                    out.write(buffer, 0, len);
                    total += len;
                    int percent = (int)(total * 80 / (1024L*1024*1024*2)); // تقدير 2GB
                    updateProgress("Copying... " + (total/1024/1024) + "MB", Math.min(percent, 80));
                }
                in.close();
                out.close();
                if (Thread.currentThread().isInterrupted()) return;

                updateProgress("Extracting...", 80);
                File installDir = new File(INSTALL_PATH);
                installDir.mkdirs();

                // استخدام tar لفك الضغط
                ProcessBuilder pb = new ProcessBuilder(
                    PROOT_PATH,
                    "-r", installDir.getAbsolutePath(),
                    "/bin/tar", "-xzf", IMAGE_PATH,
                    "-C", "/"
                );
                pb.redirectErrorStream(true);
                Process process = pb.start();
                process.waitFor();

                updateProgress("Finalizing...", 95);

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
