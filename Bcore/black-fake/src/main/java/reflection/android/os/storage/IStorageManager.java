package reflection.android.os.storage;

import android.os.IBinder;
import android.os.IInterface;

import reflection.MirrorReflection;

public class IStorageManager {
    public static class Stub {
        public static final MirrorReflection REF = MirrorReflection.on("android.os.storage.IStorageManager$Stub");

        public static MirrorReflection.StaticMethodWrapper<IInterface> asInterface = REF.staticMethod("asInterface", IBinder.class);
    }
}