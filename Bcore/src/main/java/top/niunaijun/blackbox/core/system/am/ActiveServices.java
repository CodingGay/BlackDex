package top.niunaijun.blackbox.core.system.am;

import android.content.Intent;
import android.content.pm.ResolveInfo;
import android.os.IBinder;
import android.util.Log;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

import top.niunaijun.blackbox.core.system.pm.BPackageManagerService;

/**
 * Created by Milk on 4/7/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class ActiveServices {
    public static final String TAG = "ActiveServices";

    private final Map<Intent.FilterComparison, RunningServiceRecord> mRunningServiceRecords = new HashMap<>();
    private final Map<IBinder, ConnectedServiceRecord> mConnectedServices = new HashMap<>();

    public void startService(Intent intent, String resolvedType, int userId) {

    }

    public int stopService(Intent intent, String resolvedType, int userId) {
//        ResolveInfo resolveInfo = resolveService(intent, resolvedType, userId);
        synchronized (mRunningServiceRecords) {
            RunningServiceRecord runningServiceRecord = findRunningServiceRecord(intent);
            if (runningServiceRecord == null) {
                return 0;
            }
            if (runningServiceRecord.mBindCount.get() > 0) {
                Log.d(TAG, "There are also connections");
                return 0;
            }

            runningServiceRecord.mStartId.set(0);
        }
        return 0;
    }

    public void unbindService(IBinder binder, int userId) {
        ConnectedServiceRecord connectedService = mConnectedServices.get(binder);
        if (connectedService == null) {
            return;
        }
        RunningServiceRecord runningServiceRecord = getOrCreateRunningServiceRecord(connectedService.mIntent);
        runningServiceRecord.mConnectedServiceRecord = null;
        runningServiceRecord.mBindCount.decrementAndGet();
        mConnectedServices.remove(binder);
    }

    public void onStartCommand(Intent proxyIntent, int userId) {
    }

    public void onServiceDestroy(Intent proxyIntent, int userId) {
    }

    private RunningServiceRecord getOrCreateRunningServiceRecord(Intent intent) {
        RunningServiceRecord runningServiceRecord = findRunningServiceRecord(intent);
        if (runningServiceRecord == null) {
            runningServiceRecord = new RunningServiceRecord();
            mRunningServiceRecords.put(new Intent.FilterComparison(intent), runningServiceRecord);
        }
        return runningServiceRecord;
    }

    private RunningServiceRecord findRunningServiceRecord(Intent intent) {
        return mRunningServiceRecords.get(new Intent.FilterComparison(intent));
    }

    private ResolveInfo resolveService(Intent intent, String resolvedType, int userId) {
        return BPackageManagerService.get().resolveService(intent, 0, resolvedType, userId);
    }

    private ConnectedServiceRecord findConnectedServiceRecord(Intent intent) {
        RunningServiceRecord runningServiceRecord = mRunningServiceRecords.get(intent);
        if (runningServiceRecord == null)
            return null;
        return runningServiceRecord.mConnectedServiceRecord;
    }

    public static class RunningServiceRecord {
        // onStartCommand startId
        private AtomicInteger mStartId = new AtomicInteger(1);
        private AtomicInteger mBindCount = new AtomicInteger(0);
        // 正在连接的服务
        private ConnectedServiceRecord mConnectedServiceRecord;

        public int getAndIncrementStartId() {
            return mStartId.getAndIncrement();
        }

        public int decrementBindCountAndGet() {
            return mBindCount.decrementAndGet();
        }

        public int incrementBindCountAndGet() {
            return mBindCount.incrementAndGet();
        }
    }

    public static class ConnectedServiceRecord {
        private IBinder mIBinder;
        private Intent mIntent;
    }
}
