package mirror.android.content.pm;

import android.os.IInterface;

import mirror.MirrorReflection;

public class ILauncherApps {

    public static final class Stub {
        public static final MirrorReflection REF = MirrorReflection.on("android.content.pm.ILauncherApps$Stub");
        public static MirrorReflection.StaticMethodWrapper<IInterface> asInterface = REF.staticMethod("asInterface");
    }
}
