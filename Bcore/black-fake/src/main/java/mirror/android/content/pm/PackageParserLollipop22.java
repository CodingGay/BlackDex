package mirror.android.content.pm;

import android.content.pm.PackageParser;

import java.io.File;

import mirror.MirrorReflection;

public class PackageParserLollipop22 {
    public static final MirrorReflection REF = MirrorReflection.on(android.content.pm.PackageParser.class);

    public static MirrorReflection.MethodWrapper<Void> collectCertificates = REF.method("collectCertificates", PackageParser.Package.class, int.class);
    public static MirrorReflection.ConstructorWrapper<PackageParser> constructor = REF.constructor();
    public static MirrorReflection.MethodWrapper<PackageParser.Package> parsePackage = REF.method("parsePackage", File.class, int.class);
}
