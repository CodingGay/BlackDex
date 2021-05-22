package top.niunaijun.blackbox.core.system.dump;

import android.os.IBinder;
import android.os.RemoteException;

import java.util.ArrayList;
import java.util.List;

import top.niunaijun.blackbox.entity.dump.DumpResult;

/**
 * Created by Milk on 2021/5/22.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class BDumpManagerService extends IBDumpManagerService.Stub {
    private static final BDumpManagerService sService = new BDumpManagerService();
    private final List<IBinder> mMonitors = new ArrayList<>();

    public static BDumpManagerService get() {
        return sService;
    }

    @Override
    public void registerMonitor(IBinder monitor) {
        try {
            monitor.linkToDeath(new DeathRecipient() {
                @Override
                public void binderDied() {
                    monitor.unlinkToDeath(this, 0);
                    mMonitors.remove(monitor);
                }
            }, 0);
        } catch (RemoteException ignored) {
        }
        mMonitors.add(monitor);
    }

    @Override
    public void unregisterMonitor(IBinder monitor) {
        mMonitors.remove(monitor);
    }

    @Override
    public void noticeMonitor(DumpResult result) throws RemoteException {
        for (IBinder monitor : mMonitors) {
            if (monitor.isBinderAlive()) {
                IBDumpMonitor ibDumpMonitor = IBDumpMonitor.Stub.asInterface(monitor);
                ibDumpMonitor.onDump(result);
            }
        }
    }
}
