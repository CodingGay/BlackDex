package top.niunaijun.blackbox.core.system.pm;

import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageParser;
import android.os.Parcel;
import android.os.Process;
import android.util.ArrayMap;
import android.util.AtomicFile;

import java.io.File;
import java.io.FileOutputStream;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import top.niunaijun.blackbox.core.env.BEnvironment;
import top.niunaijun.blackbox.BlackBoxCore;
import top.niunaijun.blackbox.entity.pm.InstallOption;
import top.niunaijun.blackbox.core.system.BProcessManager;
import top.niunaijun.blackbox.core.system.user.BUserHandle;
import top.niunaijun.blackbox.utils.FileUtils;
import top.niunaijun.blackbox.utils.Slog;

/**
 * Created by Milk on 4/13/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
/*public*/ class Settings {
    public static final String TAG = "Settings";

    final ArrayMap<String, BPackageSettings> mPackages = new ArrayMap<>();
    private final Map<String, Integer> mAppIds = new HashMap<>();
    private int mCurrUid = 0;

    public Settings() {
        synchronized (mPackages) {
            loadUidLP();
        }
    }

    BPackageSettings getPackageLPw(String name, PackageParser.Package aPackage) {
        BPackageSettings pkgSettings;
        BPackageSettings origSettings = new BPackageSettings();
        origSettings.pkg = new BPackage(aPackage);
        origSettings.pkg.mExtras = origSettings;
        origSettings.pkg.applicationInfo = PackageManagerCompat.generateApplicationInfo(origSettings.pkg, 0, BPackageUserState.create(), 0);
        synchronized (mPackages) {
            pkgSettings = mPackages.get(name);
            if (pkgSettings != null) {
                origSettings.appId = pkgSettings.appId;
                origSettings.userState = pkgSettings.userState;
            } else {
                boolean b = registerAppIdLPw(origSettings);
                if (!b) {
                    throw new RuntimeException("registerAppIdLPw err.");
                }
            }
        }
        return origSettings;
    }

    boolean registerAppIdLPw(BPackageSettings p) {
        boolean createdNew = false;
        if (p.appId == 0) {
            // Assign new user ID
            p.appId = acquireAndRegisterNewAppIdLPw(p);
            createdNew = true;
        }
        if (p.appId < 0) {
            createdNew = false;
//            PackageManagerService.reportSettingsProblem(Log.WARN,
//                    "Package " + p.name + " could not be assigned a valid UID");
//            throw new PackageManagerException(INSTALL_FAILED_INSUFFICIENT_STORAGE,
//                    "Package " + p.name + " could not be assigned a valid UID");
        }
        saveUidLP();
        return createdNew;
    }

    private int acquireAndRegisterNewAppIdLPw(BPackageSettings obj) {
        // Let's be stupidly inefficient for now...
        Integer integer = mAppIds.get(obj.pkg.packageName);
        if (integer != null)
            return integer;

        if (mCurrUid >= Process.LAST_APPLICATION_UID) {
            return -1;
        }
        mCurrUid++;
        mAppIds.put(obj.pkg.packageName, mCurrUid);
        return Process.FIRST_APPLICATION_UID + mCurrUid;
    }

    private void saveUidLP() {
        Parcel parcel = Parcel.obtain();
        FileOutputStream fileOutputStream = null;
        AtomicFile atomicFile = new AtomicFile(BEnvironment.getUidConf());
        try {
            Set<String> pkgName = mPackages.keySet();
            for (String s : new HashSet<>(mAppIds.keySet())) {
                if (!pkgName.contains(s)) {
                    mAppIds.remove(s);
                }
            }
            parcel.writeInt(mCurrUid);
            parcel.writeMap(mAppIds);

            fileOutputStream = atomicFile.startWrite();
            FileUtils.writeParcelToOutput(parcel, fileOutputStream);
            atomicFile.finishWrite(fileOutputStream);
        } catch (Exception e) {
            e.printStackTrace();
            atomicFile.failWrite(fileOutputStream);
        } finally {
            parcel.recycle();
        }
    }

    private void loadUidLP() {
        Parcel parcel = Parcel.obtain();
        try {
            byte[] uidBytes = FileUtils.toByteArray(BEnvironment.getUidConf());
            parcel.unmarshall(uidBytes, 0, uidBytes.length);
            parcel.setDataPosition(0);

            mCurrUid = parcel.readInt();
            HashMap hashMap = parcel.readHashMap(HashMap.class.getClassLoader());
            synchronized (mAppIds) {
                mAppIds.clear();
                mAppIds.putAll(hashMap);
            }
        } catch (Exception e) {
//            e.printStackTrace();
        } finally {
            parcel.recycle();
        }
    }

    public void scanPackage() {
        synchronized (mPackages) {
            File appRootDir = BEnvironment.getAppRootDir();
            FileUtils.mkdirs(appRootDir);
            File[] apps = appRootDir.listFiles();
            for (File app : apps) {
                if (!app.isDirectory()) {
                    continue;
                }
                updatePackageLP(app);
            }
        }
    }

    private void updatePackageLP(File app) {
        String packageName = app.getName();
        Parcel packageSettingsIn = Parcel.obtain();
        File packageConf = BEnvironment.getPackageConf(packageName);
        try {
            byte[] bPackageSettingsBytes = FileUtils.toByteArray(packageConf);

            packageSettingsIn.unmarshall(bPackageSettingsBytes, 0, bPackageSettingsBytes.length);
            packageSettingsIn.setDataPosition(0);

            BPackageSettings bPackageSettings = new BPackageSettings(packageSettingsIn);
            if (bPackageSettings.installOption.isFlag(InstallOption.FLAG_SYSTEM)) {
                PackageInfo packageInfo = BlackBoxCore.getPackageManager().getPackageInfo(packageName, PackageManager.GET_META_DATA);
                String currPackageSourcePath = packageInfo.applicationInfo.sourceDir;
                if (!currPackageSourcePath.equals(bPackageSettings.pkg.baseCodePath)) {
                    // update baseCodePath And Re install
                    BProcessManager.get().killAllByPackageName(bPackageSettings.pkg.packageName);
                    bPackageSettings.pkg.baseCodePath = currPackageSourcePath;
                    BPackageInstallerService.get().updatePackage(bPackageSettings);
                }
            }
            bPackageSettings.pkg.mExtras = bPackageSettings;
            bPackageSettings.pkg.applicationInfo = PackageManagerCompat.generateApplicationInfo(bPackageSettings.pkg, 0, BPackageUserState.create(), 0);
            bPackageSettings.save();
            mPackages.put(bPackageSettings.pkg.packageName, bPackageSettings);
            Slog.d(TAG, "loaded Package: " + packageName);
        } catch (Throwable e) {
            e.printStackTrace();
            // bad package
            FileUtils.deleteDir(app);
            mPackages.remove(packageName);
            BProcessManager.get().killAllByPackageName(packageName);
            BPackageManagerService.get().onPackageUninstalled(packageName, BUserHandle.USER_ALL);
            Slog.d(TAG, "bad Package: " + packageName);
        } finally {
            packageSettingsIn.recycle();
        }
    }
}
