package mirror.android.app;

import mirror.MirrorReflection;

public class IActivityManager {
    public static final MirrorReflection REF = MirrorReflection.on("android.app.IActivityManager");

    public static MirrorReflection.MethodWrapper<Integer> startActivity = REF.method("startActivity");
}
