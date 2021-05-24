package reflection.android.content.pm;

import android.util.DisplayMetrics;
import java.io.File;
import reflection.MirrorReflection;

public class PackageParser {
    public static final MirrorReflection REF = MirrorReflection.on(android.content.pm.PackageParser.class);

    public static MirrorReflection.MethodWrapper<Void> collectCertificates = REF.method("collectCertificates", android.content.pm.PackageParser.Package.class, int.class);
    public static MirrorReflection.ConstructorWrapper<android.content.pm.PackageParser> constructor = REF.constructor(String.class);
    public static MirrorReflection.MethodWrapper<android.content.pm.PackageParser.Package> parsePackage = REF.method("parsePackage", File.class, String.class, DisplayMetrics.class, int.class);
}
