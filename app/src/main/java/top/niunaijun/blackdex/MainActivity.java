package top.niunaijun.blackdex;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;

import java.io.File;

import top.niunaijun.blackbox.BlackDexCore;
import top.niunaijun.blackbox.core.system.dump.IBDumpMonitor;
import top.niunaijun.blackbox.entity.dump.DumpResult;

public class MainActivity extends AppCompatActivity {
    public static final String TAG = "MainActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        // 注册dump监听
        BlackDexCore.get().registerDumpMonitor(mMonitor);

        findViewById(R.id.btn_click).setOnClickListener(v -> {
            // 此方法会阻塞
            boolean b = BlackDexCore.get().dumpDex(new File("/sdcard/huluxia.apk"));
            if (!b) {
                Log.d(TAG, "dumpDex: error.");
            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        BlackDexCore.get().unregisterDumpMonitor(mMonitor);
    }

    private final IBDumpMonitor mMonitor = new IBDumpMonitor.Stub() {
        @Override
        public void onDump(DumpResult result) {
            Log.d(TAG, "onDump: " + result.toString());
        }
    };
}
