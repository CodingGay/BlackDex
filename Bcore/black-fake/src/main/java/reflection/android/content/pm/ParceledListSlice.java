package reflection.android.content.pm;

import android.os.Parcelable;

import reflection.MirrorReflection;

public class ParceledListSlice {
    public static final MirrorReflection REF = MirrorReflection.on("android.content.pm.ParceledListSlice");
    public static MirrorReflection.MethodWrapper<Boolean> append = REF.method("append");
    public static MirrorReflection.ConstructorWrapper<Parcelable> constructor = REF.constructor();

    public static MirrorReflection.MethodWrapper<Void> setLastSlice = REF.method("setLastSlice");
}
