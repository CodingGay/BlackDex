package reflection.android.app;

import android.os.IBinder;
import android.os.IInterface;

import reflection.MirrorReflection;

public class IApplicationThreadOreo {
    public static final class Stub {
        public static final MirrorReflection REF = MirrorReflection.on("android.app.IApplicationThread$Stub");
        public static MirrorReflection.StaticMethodWrapper<IInterface> asInterface = REF.staticMethod("asInterface", IBinder.class);
    }
}
