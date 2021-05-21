package top.niunaijun.blackdex;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;

import java.io.File;

import top.niunaijun.blackbox.BlackBoxCore;
import top.niunaijun.blackbox.entity.pm.InstallResult;

public class MainActivity extends AppCompatActivity {
    public static final String TAG = "MainActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        findViewById(R.id.btn_click).setOnClickListener(v -> {
            String hlx = "com.huluxia.gametools";
            boolean installed = BlackBoxCore.get().isInstalled(hlx);
            if (!installed) {
                InstallResult installResult = BlackBoxCore.get().installPackage(new File("/sdcard/huluxia.apk"));
                Log.d(TAG, "onClick: " + installResult.toString());
            }
            BlackBoxCore.get().launchApk(hlx);
        });
    }
}