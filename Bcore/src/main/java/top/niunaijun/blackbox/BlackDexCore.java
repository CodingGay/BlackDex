package top.niunaijun.blackbox;

import android.content.Context;

import java.io.File;

import top.niunaijun.blackbox.app.configuration.ClientConfiguration;
import top.niunaijun.blackbox.core.system.dump.IBDumpMonitor;
import top.niunaijun.blackbox.entity.pm.InstallResult;

/**
 * Created by Milk on 2021/5/22.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class BlackDexCore {
    public static final String TAG = "BlackBoxCore";

    private static final BlackDexCore sBlackDexCore = new BlackDexCore();

    public static BlackDexCore get() {
        return sBlackDexCore;
    }

    public void doAttachBaseContext(Context context, ClientConfiguration clientConfiguration) {
        BlackBoxCore.get().doAttachBaseContext(context, clientConfiguration);
    }

    public void doCreate() {
        BlackBoxCore.get().doCreate();
    }

    public boolean dumpDex(String packageName) {
        InstallResult installResult = BlackBoxCore.get().installPackage(packageName);
        if (installResult.success) {
            return BlackBoxCore.get().launchApk(packageName);
        } else {
            return false;
        }
    }

    public boolean dumpDex(File file) {
        InstallResult installResult = BlackBoxCore.get().installPackage(file);
        if (installResult.success) {
            return BlackBoxCore.get().launchApk(installResult.packageName);
        } else {
            return false;
        }
    }

    public void registerDumpMonitor(IBDumpMonitor monitor) {
        BlackBoxCore.getBDumpManager().registerMonitor(monitor.asBinder());
    }

    public void unregisterDumpMonitor(IBDumpMonitor monitor) {
        BlackBoxCore.getBDumpManager().unregisterMonitor(monitor.asBinder());
    }
}
