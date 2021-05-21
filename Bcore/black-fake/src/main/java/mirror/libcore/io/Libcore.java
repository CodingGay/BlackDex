package mirror.libcore.io;

import mirror.MirrorReflection;

public class Libcore {
    public static final String NAME = "libcore.io.Libcore";
    public static final MirrorReflection REF = MirrorReflection.on(NAME);

    public static MirrorReflection.FieldWrapper<Object> os = REF.field("os");
}
