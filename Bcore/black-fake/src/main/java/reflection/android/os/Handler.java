package reflection.android.os;

import reflection.MirrorReflection;

public class Handler {
    public static final MirrorReflection REF = MirrorReflection.on(android.os.Handler.class);

    public static MirrorReflection.FieldWrapper<android.os.Handler.Callback> mCallback = REF.field("mCallback");
}
