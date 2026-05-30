package com.vviewer;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;

public class SettingsActivity extends Activity {
    private TextView statusText;
    private Button selectFileBtn, installBtn, deleteBtn;
    private Uri selectedFileUri = null;
    private static final int PICK_FILE = 1;
    private static final String IMAGE_PATH = Environment.getExternalStorageDirectory() + "/V-Viewer/rootfs.tar.gz";
    private static final String INSTALL_PATH = "/data/data/com.vviewer/files/ubuntu";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.settings);

        statusText = findViewById(R.id.statusText);
        selectFileBtn = findViewById(R.id.selectFileBtn);
        installBtn = findViewById(R.id.installBtn);
        deleteBtn = findViewById(R.id.deleteBtn);

        updateStatus();

        selectFileBtn.setOnClickListener(v -> {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("*/*");
            startActivityForResult(intent, PICK_FILE);
        });

        installBtn.setOnClickListener(v -> {
            if (selectedFileUri != null) {
                installImage();
            } else {
                Toast.makeText(this, "Select a file first", Toast.LENGTH_SHORT).show();
            }
        });

        deleteBtn.setOnClickListener(v -> {
            deleteInstallation();
        });
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == PICK_FILE && resultCode == RESULT_OK && data != null) {
            selectedFileUri = data.getData();
            Toast.makeText(this, "File selected: " + selectedFileUri.getLastPathSegment(), Toast.LENGTH_SHORT).show();
        }
    }

    private void installImage() {
        try {
            File destDir = new File(Environment.getExternalStorageDirectory(), "V-Viewer");
            destDir.mkdirs();
            File destFile = new File(IMAGE_PATH);
            FileInputStream in = (FileInputStream) getContentResolver().openInputStream(selectedFileUri);
            FileOutputStream out = new FileOutputStream(destFile);
            byte[] buffer = new byte[8192];
            int len;
            while ((len = in.read(buffer)) > 0) {
                out.write(buffer, 0, len);
            }
            in.close();
            out.close();
            Toast.makeText(this, "Image copied. Extracting...", Toast.LENGTH_SHORT).show();
            // هنا سيتم استدعاء أمر فك الضغط عبر proot أو كود C++ لاحقاً
            updateStatus();
        } catch (Exception e) {
            Toast.makeText(this, "Error: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }
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
