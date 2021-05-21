package mirror.android.app;

import android.os.IBinder;
import android.os.IInterface;

import mirror.MirrorReflection;

public class IActivityTaskManager {

    public static class Stub {
        public static final MirrorReflection REF = MirrorReflection.on("android.app.IActivityTaskManager$Stub");

        public static MirrorReflection.StaticMethodWrapper<IInterface> asInterface = REF.staticMethod("asInterface", IBinder.class);
    }
}
