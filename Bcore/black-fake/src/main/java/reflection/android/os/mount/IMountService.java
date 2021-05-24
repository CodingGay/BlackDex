package reflection.android.os.mount;

import android.os.IBinder;
import android.os.IInterface;

import reflection.MirrorReflection;

public class IMountService {
    public static class Stub {
        public static final MirrorReflection REF = MirrorReflection.on("android.os.storage.IMountService$Stub");
        public static MirrorReflection.StaticMethodWrapper<IInterface> asInterface = REF.staticMethod("asInterface", IBinder.class);
    }
}
