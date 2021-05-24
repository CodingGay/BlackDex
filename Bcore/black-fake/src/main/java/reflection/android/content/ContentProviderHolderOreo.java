package reflection.android.content;

import android.content.pm.ProviderInfo;
import android.os.IInterface;

import reflection.MirrorReflection;

public class ContentProviderHolderOreo {
    public static final MirrorReflection REF = MirrorReflection.on("android.app.ContentProviderHolder");

    public static MirrorReflection.FieldWrapper<ProviderInfo> info = REF.field("info");
    public static MirrorReflection.FieldWrapper<IInterface> provider = REF.field("provider");
}
