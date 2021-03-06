package reflection.android.content.pm;

import android.content.pm.PackageParser;

import reflection.MirrorReflection;

public class PackageParserPie {
    public static final MirrorReflection REF = MirrorReflection.on(android.content.pm.PackageParser.class);

    public static MirrorReflection.StaticMethodWrapper<Void> collectCertificates = REF.staticMethod("collectCertificates", PackageParser.Package.class, boolean.class);
}
