package mirror.android.app;


import android.app.Activity;
import android.app.Application;
import android.app.Instrumentation;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.ProviderInfo;
import android.os.Handler;
import android.os.IBinder;
import android.os.IInterface;

import java.util.List;

import mirror.MirrorReflection;

/**
 * Created by Milk on 5/20/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class ActivityThread {
    public static final String NAME = "android.app.ActivityThread";
    public static final MirrorReflection REF = MirrorReflection.on(NAME);

    public static MirrorReflection.StaticMethodWrapper<Object> currentActivityThread = REF.staticMethod("currentActivityThread");
    public static MirrorReflection.FieldWrapper<Object> mBoundApplication = REF.field("mBoundApplication");
    public static MirrorReflection.FieldWrapper<Handler> mH = REF.field("mH");
    public static MirrorReflection.FieldWrapper<Application> mInitialApplication = REF.field("mInitialApplication");
    public static MirrorReflection.FieldWrapper<Instrumentation> mInstrumentation = REF.field("mInstrumentation");
    public static MirrorReflection.FieldWrapper<IInterface> sPackageManager = REF.field("sPackageManager");
    public static MirrorReflection.MethodWrapper<IBinder> getApplicationThread = REF.method("getApplicationThread");
    public static MirrorReflection.MethodWrapper<Object> getSystemContext = REF.method("getSystemContext");


    public static class ActivityClientRecord {
        public static final MirrorReflection REF = MirrorReflection.on("android.app.ActivityThread$ActivityClientRecord");
        public static MirrorReflection.FieldWrapper<Activity> activity = REF.field("activity");
        public static MirrorReflection.FieldWrapper<ActivityInfo> activityInfo = REF.field("activityInfo");
        public static MirrorReflection.FieldWrapper<Intent> intent = REF.field("intent");
    }

    public static class AppBindData {
        public static final MirrorReflection REF = MirrorReflection.on("android.app.ActivityThread$AppBindData");
        public static MirrorReflection.FieldWrapper<ApplicationInfo> appInfo = REF.field("appInfo");
        public static MirrorReflection.FieldWrapper<Object> info = REF.field("info");
        public static MirrorReflection.FieldWrapper<String> processName = REF.field("processName");
        public static MirrorReflection.FieldWrapper<ComponentName> instrumentationName = REF.field("instrumentationName");
        public static MirrorReflection.FieldWrapper<List<ProviderInfo>> providers = REF.field("providers");
    }

    public static class H {
        public static final MirrorReflection REF = MirrorReflection.on("android.app.ActivityThread$H");
        public static MirrorReflection.FieldWrapper<Integer> LAUNCH_ACTIVITY = REF.field("LAUNCH_ACTIVITY");
        public static MirrorReflection.FieldWrapper<Integer> EXECUTE_TRANSACTION = REF.field("EXECUTE_TRANSACTION");
    }
}
