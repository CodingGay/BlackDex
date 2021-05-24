package reflection.com.android.internal.telephony;

import android.os.IBinder;
import android.os.IInterface;

import reflection.MirrorReflection;

public class ITelephony {
    public static class Stub {
        public static final MirrorReflection REF = MirrorReflection.on("com.android.internal.telephony.ITelephony$Stub");

        public static MirrorReflection.StaticMethodWrapper<IInterface> asInterface = REF.staticMethod("asInterface", IBinder.class);
    }
}
