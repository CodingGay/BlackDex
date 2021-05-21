package mirror.android.app.job;

import android.os.IBinder;
import android.os.IInterface;

import mirror.MirrorReflection;

public class IJobScheduler {
    public static class Stub {
        public static final MirrorReflection REF = MirrorReflection.on("android.app.job.IJobScheduler$Stub");
        public static MirrorReflection.StaticMethodWrapper<IInterface> asInterface = REF.staticMethod("asInterface", IBinder.class);
    }
}
