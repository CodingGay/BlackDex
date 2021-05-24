package reflection.libcore.io;

import reflection.MirrorReflection;

public class Libcore {
    public static final String NAME = "libcore.io.Libcore";
    public static final MirrorReflection REF = MirrorReflection.on(NAME);

    public static MirrorReflection.FieldWrapper<Object> os = REF.field("os");
}
