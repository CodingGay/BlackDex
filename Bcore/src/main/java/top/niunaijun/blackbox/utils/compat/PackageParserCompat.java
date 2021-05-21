package top.niunaijun.blackbox.utils.compat;

import android.content.pm.PackageParser;
import android.content.pm.PackageParser.Package;
import android.util.DisplayMetrics;
import java.io.File;

import mirror.android.content.pm.PackageParserLollipop;
import mirror.android.content.pm.PackageParserLollipop22;
import mirror.android.content.pm.PackageParserMarshmallow;
import mirror.android.content.pm.PackageParserNougat;
import mirror.android.content.pm.PackageParserPie;

public class PackageParserCompat {

    public static PackageParser createParser(File packageFile) {
        if (BuildCompat.isM()) {
            return PackageParserMarshmallow.constructor.newInstance();
        } else if (BuildCompat.isL_MR1()) {
            return PackageParserLollipop22.constructor.newInstance();
        } else if (BuildCompat.isL()) {
            return PackageParserLollipop.constructor.newInstance();
        } else {
            return mirror.android.content.pm.PackageParser.constructor.newInstance(packageFile.getAbsolutePath());
        }
    }

    public static Package parsePackage(PackageParser parser, File packageFile, int flags) throws Throwable {
        if (BuildCompat.isM()) {
            return PackageParserMarshmallow.parsePackage.callWithException(parser, packageFile, flags);
        } else if (BuildCompat.isL_MR1()) {
            return PackageParserLollipop22.parsePackage.callWithException(parser, packageFile, flags);
        } else if (BuildCompat.isL()) {
            return PackageParserLollipop.parsePackage.callWithException(parser, packageFile, flags);
        } else {
            return mirror.android.content.pm.PackageParser.parsePackage.callWithException(parser, packageFile, null,
                    new DisplayMetrics(), flags);
        }
    }

    public static void collectCertificates(PackageParser parser, Package p, int flags) throws Throwable {
        if (BuildCompat.isPie()) {
            PackageParserPie.collectCertificates.callWithException(p, true/*skipVerify*/);
        } else if (BuildCompat.isN()) {
            PackageParserNougat.collectCertificates.callWithException(p, flags);
        } else if (BuildCompat.isM()) {
            PackageParserMarshmallow.collectCertificates.callWithException(parser, p, flags);
        } else if (BuildCompat.isL_MR1()) {
            PackageParserLollipop22.collectCertificates.callWithException(parser, p, flags);
        } else if (BuildCompat.isL()) {
            PackageParserLollipop.collectCertificates.callWithException(parser, p, flags);
        } else {
            mirror.android.content.pm.PackageParser.collectCertificates.call(parser, p, flags);
        }
    }
}
