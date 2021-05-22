package top.niunaijun.blackbox.fake.frameworks;

import android.os.IBinder;
import android.os.RemoteException;
import android.os.storage.StorageVolume;

import top.niunaijun.blackbox.BlackBoxCore;
import top.niunaijun.blackbox.core.system.ServiceManager;
import top.niunaijun.blackbox.core.system.dump.IBDumpManagerService;
import top.niunaijun.blackbox.core.system.os.IBStorageManagerService;
import top.niunaijun.blackbox.entity.dump.DumpResult;

/**
 * Created by Milk on 4/14/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class BDumpManager {
    private static final BDumpManager sDumpManager = new BDumpManager();
    private IBDumpManagerService mService;

    public static BDumpManager get() {
        return sDumpManager;
    }

    public void registerMonitor(IBinder monitor) {
        try {
            getService().registerMonitor(monitor);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public void unregisterMonitor(IBinder monitor) {
        try {
            getService().unregisterMonitor(monitor);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public void noticeMonitor(DumpResult result) {
        try {
            getService().noticeMonitor(result);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    private IBDumpManagerService getService() {
        if (mService != null && mService.asBinder().isBinderAlive()) {
            return mService;
        }
        mService = IBDumpManagerService.Stub.asInterface(BlackBoxCore.get().getService(ServiceManager.DUMP_MANAGER));
        return getService();
    }
}
