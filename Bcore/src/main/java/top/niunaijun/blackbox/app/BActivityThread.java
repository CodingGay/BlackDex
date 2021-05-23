package top.niunaijun.blackbox.app;

import android.app.Application;
import android.app.Instrumentation;
import android.content.ComponentName;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ProviderInfo;
import android.os.Build;
import android.os.ConditionVariable;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Process;
import android.util.Log;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;

import mirror.android.app.ActivityThread;
import mirror.android.app.ContextImpl;
import mirror.android.app.LoadedApk;
import top.niunaijun.blackbox.core.IBActivityThread;
import top.niunaijun.blackbox.core.VMCore;
import top.niunaijun.blackbox.entity.AppConfig;
import top.niunaijun.blackbox.core.IOCore;
import top.niunaijun.blackbox.entity.dump.DumpResult;
import top.niunaijun.blackbox.utils.Slog;
import top.niunaijun.blackbox.utils.compat.ContextCompat;
import top.niunaijun.blackbox.BlackBoxCore;

/**
 * Created by Milk on 3/31/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class BActivityThread extends IBActivityThread.Stub {
    public static final String TAG = "BActivityThread";

    private static BActivityThread sBActivityThread;
    private AppBindData mBoundApplication;
    private Application mInitialApplication;
    private AppConfig mAppConfig;
    private final List<ProviderInfo> mProviders = new ArrayList<>();

    public static BActivityThread currentActivityThread() {
        if (sBActivityThread == null) {
            synchronized (BActivityThread.class) {
                if (sBActivityThread == null) {
                    sBActivityThread = new BActivityThread();
                }
            }
        }
        return sBActivityThread;
    }

    public static synchronized AppConfig getAppConfig() {
        return currentActivityThread().mAppConfig;
    }

    public static List<ProviderInfo> getProviders() {
        return currentActivityThread().mProviders;
    }

    public static String getAppProcessName() {
        if (getAppConfig() != null) {
            return getAppConfig().processName;
        } else if (currentActivityThread().mBoundApplication != null) {
            return currentActivityThread().mBoundApplication.processName;
        } else {
            return null;
        }
    }

    public static String getAppPackageName() {
        if (getAppConfig() != null) {
            return getAppConfig().packageName;
        } else if (currentActivityThread().mInitialApplication != null) {
            return currentActivityThread().mInitialApplication.getPackageName();
        } else {
            return null;
        }
    }

    public static Application getApplication() {
        return currentActivityThread().mInitialApplication;
    }

    public static int getAppPid() {
        return getAppConfig() == null ? -1 : getAppConfig().bpid;
    }

    public static int getAppUid() {
        return getAppConfig() == null ? 10000 : getAppConfig().buid;
    }

    public static int getBaseAppUid() {
        return getAppConfig() == null ? 10000 : getAppConfig().baseBUid;
    }

    public static int getUid() {
        return getAppConfig() == null ? -1 : getAppConfig().uid;
    }

    public static int getUserId() {
        return getAppConfig() == null ? 0 : getAppConfig().userId;
    }

    public void initProcess(AppConfig appConfig) {
        if (this.mAppConfig != null) {
            throw new RuntimeException("reject init process: " + appConfig.processName + ", this process is : " + this.mAppConfig.processName);
        }
        this.mAppConfig = appConfig;
    }

    public boolean isInit() {
        return mBoundApplication != null;
    }

    public void bindApplication(final String packageName, final String processName) {
        if (mAppConfig == null) {
            return;
        }
        if (Looper.myLooper() != Looper.getMainLooper()) {
            final ConditionVariable conditionVariable = new ConditionVariable();
            new Handler(Looper.getMainLooper()).post(() -> {
                handleBindApplication(packageName, processName);
                conditionVariable.open();
            });
            conditionVariable.block();
        } else {
            handleBindApplication(packageName, processName);
        }
    }

    public synchronized void handleBindApplication(String packageName, String processName) {
        DumpResult result = new DumpResult();
        result.packageName = packageName;
        result.dir = new File(BlackBoxCore.get().getDexDumpDir(), packageName).getAbsolutePath();
        try {
            PackageInfo packageInfo = BlackBoxCore.getBPackageManager().getPackageInfo(packageName, PackageManager.GET_PROVIDERS, BActivityThread.getUserId());
            if (packageInfo == null)
                return;
            ApplicationInfo applicationInfo = packageInfo.applicationInfo;
            if (packageInfo.providers == null) {
                packageInfo.providers = new ProviderInfo[]{};
            }
            mProviders.addAll(Arrays.asList(packageInfo.providers));

            Object boundApplication = ActivityThread.mBoundApplication.get(BlackBoxCore.mainThread());

            Context packageContext = createPackageContext(applicationInfo);
            Object loadedApk = ContextImpl.mPackageInfo.get(packageContext);
            LoadedApk.mSecurityViolation.set(loadedApk, false);
            // fix applicationInfo
            LoadedApk.mApplicationInfo.set(loadedApk, applicationInfo);

            VMCore.init(Build.VERSION.SDK_INT);
            assert packageContext != null;
            IOCore.get().enableRedirect(packageContext);

            AppBindData bindData = new AppBindData();
            bindData.appInfo = applicationInfo;
            bindData.processName = processName;
            bindData.info = loadedApk;
            bindData.providers = mProviders;

            ActivityThread.AppBindData.instrumentationName.set(boundApplication,
                    new ComponentName(bindData.appInfo.packageName, Instrumentation.class.getName()));
            ActivityThread.AppBindData.appInfo.set(boundApplication, bindData.appInfo);
            ActivityThread.AppBindData.info.set(boundApplication, bindData.info);
            ActivityThread.AppBindData.processName.set(boundApplication, bindData.processName);
            ActivityThread.AppBindData.providers.set(boundApplication, bindData.providers);

            mBoundApplication = bindData;

            Application application = null;
            BlackBoxCore.get().getAppLifecycleCallback().beforeCreateApplication(packageName, processName, packageContext);
            try {
                application = LoadedApk.makeApplication.call(loadedApk, false, null);
            } catch (Throwable e) {
//                Slog.e(TAG, "Unable to makeApplication");
//                e.printStackTrace();
            }
            mInitialApplication = application;
            if (Objects.equals(packageName, processName)) {
                ClassLoader loader;
                if (application == null) {
                    loader = LoadedApk.getClassloader.call(loadedApk);
                } else {
                    loader = application.getClassLoader();
                }
                VMCore.dumpDex(loader, processName);
            }
        } catch (Throwable e) {
            e.printStackTrace();
            result.dumpError(e.getMessage());
        } finally {
            mAppConfig = null;
            BlackBoxCore.getBDumpManager().noticeMonitor(result);
            BlackBoxCore.get().uninstallPackage(packageName);
        }
    }

    private Context createPackageContext(ApplicationInfo info) {
        try {
            return BlackBoxCore.getContext().createPackageContext(info.packageName,
                    Context.CONTEXT_INCLUDE_CODE | Context.CONTEXT_IGNORE_SECURITY);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    @Override
    public IBinder getActivityThread() {
        return ActivityThread.getApplicationThread.call(BlackBoxCore.mainThread());
    }

    @Override
    public void bindApplication() {
        if (!isInit()) {
            bindApplication(getAppPackageName(), getAppProcessName());
        }
    }

    public static class AppBindData {
        String processName;
        ApplicationInfo appInfo;
        List<ProviderInfo> providers;
        Object info;
    }
}
