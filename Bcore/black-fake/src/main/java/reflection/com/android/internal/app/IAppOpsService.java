package reflection.com.android.internal.app;

import android.os.IBinder;
import android.os.IInterface;

import reflection.MirrorReflection;


public class IAppOpsService {
    public static class Stub {
        public static final String NAME = "com.android.internal.app.IAppOpsService$Stub";
        private static final MirrorReflection REF = MirrorReflection.on(NAME);
        public static MirrorReflection.StaticMethodWrapper<IInterface> asInterface = REF.staticMethod("asInterface", IBinder.class);
    }
}