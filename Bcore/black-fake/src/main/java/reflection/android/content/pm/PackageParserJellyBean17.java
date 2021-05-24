package reflection.android.content.pm;

import android.content.pm.PackageParser;
import android.util.DisplayMetrics;

import java.io.File;

import reflection.MirrorReflection;

public class PackageParserJellyBean17 {
    public static final MirrorReflection REF = MirrorReflection.on(android.content.pm.PackageParser.class);
    public static MirrorReflection.MethodWrapper<Void> collectCertificates = REF.method("collectCertificates", PackageParser.Package.class, int.class);
    public static MirrorReflection.ConstructorWrapper<PackageParser> constructor = REF.constructor(String.class);
    public static MirrorReflection.MethodWrapper<PackageParser.Package> parsePackage = REF.method("parsePackage", File.class, String.class, DisplayMetrics.class, int.class);
}
