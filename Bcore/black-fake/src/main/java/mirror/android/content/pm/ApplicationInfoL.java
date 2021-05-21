package mirror.android.content.pm;

import android.content.pm.ApplicationInfo;

import mirror.MirrorReflection;

public class ApplicationInfoL {
    public static final MirrorReflection REF = MirrorReflection.on(ApplicationInfo.class);
    public static MirrorReflection.FieldWrapper<String> primaryCpuAbi = REF.field("primaryCpuAbi");
    public static MirrorReflection.FieldWrapper<String> scanPublicSourceDir = REF.field("scanPublicSourceDir");
    public static MirrorReflection.FieldWrapper<String> scanSourceDir = REF.field("scanSourceDir");
    public static MirrorReflection.FieldWrapper<String> secondaryCpuAbi = REF.field("secondaryCpuAbi");
    public static MirrorReflection.FieldWrapper<String[]> splitPublicSourceDirs = REF.field("splitPublicSourceDirs");
}
