// IBActivityManagerService.aidl
package top.niunaijun.blackbox.core.system.am;

import android.content.Intent;
import android.content.ComponentName;
import android.content.pm.ServiceInfo;
import android.content.pm.ProviderInfo;
import android.os.IBinder;
import java.lang.String;
import android.app.IServiceConnection;
import top.niunaijun.blackbox.entity.AppConfig;
import android.os.Bundle;

// Declare any non-default types here with import statements

interface IBActivityManagerService {
    AppConfig initProcess(String packageName, String processName, int userId);
    void restartProcess(String packageName, String processName, int userId);

    void startActivity(in Intent intent, int userId);
    int startActivityAms(int userId, in Intent intent, String resolvedType, IBinder resultTo, String resultWho, int requestCode, int flags, in Bundle options);
    int startActivities(int userId, in Intent[] intent, in String[] resolvedType, IBinder resultTo, in Bundle options);

    ComponentName startService(in Intent intent, String resolvedType, int userId);
    int stopService(in Intent intent,in String resolvedType, int userId);

    Intent bindService(in Intent service, in IBinder binder, String resolvedType, int userId);
    void unbindService(in IBinder binder, int userId);

    void onStartCommand(in Intent proxyIntent, int userId);
    void onServiceDestroy(in Intent proxyIntent, int userId);

    Intent sendBroadcast(in Intent intent, String resolvedType, int userId);

    void onActivityCreated(int taskId, IBinder token, IBinder activityRecord);
    void onActivityResumed(IBinder token);
    void onActivityDestroyed(IBinder token);
    void onFinishActivity(IBinder token);
}
