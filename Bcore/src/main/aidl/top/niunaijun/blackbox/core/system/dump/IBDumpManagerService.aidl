// IBDumpService.aidl
package top.niunaijun.blackbox.core.system.dump;

import android.os.IBinder;
import top.niunaijun.blackbox.entity.dump.DumpResult;

interface IBDumpManagerService {
    void registerMonitor(IBinder monitor);
    void unregisterMonitor(IBinder monitor);
    void noticeMonitor(in DumpResult result);
}