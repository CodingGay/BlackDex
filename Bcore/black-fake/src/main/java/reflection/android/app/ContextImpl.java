package reflection.android.app;


import android.content.pm.PackageManager;

import reflection.MirrorReflection;

public class ContextImpl {
    public static final MirrorReflection REF = MirrorReflection.on("android.app.ContextImpl");

    public static MirrorReflection.FieldWrapper<String> mBasePackageName = REF.field("mBasePackageName");
    public static MirrorReflection.FieldWrapper<Object> mPackageInfo = REF.field("mPackageInfo");
    public static MirrorReflection.FieldWrapper<PackageManager> mPackageManager = REF.field("mPackageManager");
    public static MirrorReflection.FieldWrapper<String> mOpPackageName = REF.field("mOpPackageName");
}
