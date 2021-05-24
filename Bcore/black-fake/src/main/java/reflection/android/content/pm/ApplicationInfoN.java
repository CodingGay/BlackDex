package reflection.android.content.pm;

import android.content.pm.ApplicationInfo;

import reflection.MirrorReflection;

public class ApplicationInfoN {
    public static final MirrorReflection REF = MirrorReflection.on(ApplicationInfo.class);

    public static MirrorReflection.FieldWrapper<String> deviceProtectedDataDir = REF.field("deviceProtectedDataDir");
    public static MirrorReflection.FieldWrapper<String> deviceEncryptedDataDir = REF.field("deviceEncryptedDataDir");
    public static MirrorReflection.FieldWrapper<String> credentialProtectedDataDir = REF.field("credentialProtectedDataDir");
    public static MirrorReflection.FieldWrapper<String> credentialEncryptedDataDir = REF.field("credentialEncryptedDataDir");
}
