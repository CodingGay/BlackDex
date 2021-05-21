package mirror.android.util;


import mirror.MirrorReflection;

public class Singleton {
    public static final MirrorReflection REF = MirrorReflection.on("android.util.Singleton");
    public static MirrorReflection.MethodWrapper<Object> get = REF.method("get");
    public static MirrorReflection.FieldWrapper<Object> mInstance = REF.field("mInstance");
}
