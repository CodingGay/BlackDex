package top.niunaijun.blackdex;

import android.app.Application;
import android.content.Context;

import java.io.File;

import top.niunaijun.blackbox.BlackDexCore;
import top.niunaijun.blackbox.app.configuration.ClientConfiguration;

/**
 * Created by Milk on 2021/5/20.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class App extends Application {
    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        String dir = new File(base.getExternalCacheDir().getParent(), "dump").getAbsolutePath();
        BlackDexCore.get().doAttachBaseContext(base, new ClientConfiguration() {
            @Override
            public String getHostPackageName() {
                return base.getPackageName();
            }

            @Override
            public String getDexDumpDir() {
                // 此处一定要给固定值，可以在doAttachBaseContext之前就把路径确定好。否则doAttachBaseContext后可能会遭到hook。
                return dir;
            }
        });
    }

    @Override
    public void onCreate() {
        super.onCreate();
        BlackDexCore.get().doCreate();
    }
}
