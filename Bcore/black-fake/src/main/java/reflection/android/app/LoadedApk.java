package reflection.android.app;

import android.app.Application;
import android.app.Instrumentation;
import android.content.pm.ApplicationInfo;
import reflection.MirrorReflection;

public class LoadedApk {
    public static final MirrorReflection REF = MirrorReflection.on("android.app.LoadedApk");
    public static MirrorReflection.FieldWrapper<ApplicationInfo> mApplicationInfo = REF.field("mApplicationInfo");
    public static MirrorReflection.MethodWrapper<Application> makeApplication = REF.method("makeApplication", boolean.class, Instrumentation.class);
    public static MirrorReflection.MethodWrapper<ClassLoader> getClassloader = REF.method("getClassloader");

    public static MirrorReflection.FieldWrapper<Boolean> mSecurityViolation = REF.field("mSecurityViolation");
}