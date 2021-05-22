package top.niunaijun.blackdex;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import java.io.File;
import top.niunaijun.blackbox.BlackBoxCore;

public class MainActivity extends AppCompatActivity {
    public static final String TAG = "MainActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        findViewById(R.id.btn_click).setOnClickListener(v -> {
            BlackBoxCore.get().dumpDex(new File("/sdcard/huluxia.apk"));
        });
    }
}
