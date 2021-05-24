package reflection.android.os;

import android.os.IBinder;

import reflection.MirrorReflection;

public class Bundle {
    public static final MirrorReflection REF = MirrorReflection.on(android.os.Bundle.class);

    public static MirrorReflection.MethodWrapper<Void> putIBinder = REF.method("putIBinder", String.class, IBinder.class);

    public static MirrorReflection.MethodWrapper<IBinder> getIBinder = REF.method("getIBinder", String.class);
}
