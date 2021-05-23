package mirror.android.app;

import android.content.pm.ProviderInfo;
import android.os.IInterface;

import mirror.MirrorReflection;

public class IActivityManager {
    public static final MirrorReflection REF = MirrorReflection.on("android.app.IActivityManager");

    public static MirrorReflection.MethodWrapper<Integer> startActivity = REF.method("startActivity");

    public static class ContentProviderHolder {
        public static final MirrorReflection REF = MirrorReflection.on("android.app.IActivityManager$ContentProviderHolder");
        public static MirrorReflection.FieldWrapper<ProviderInfo> info = REF.field("info");
        public static MirrorReflection.FieldWrapper<IInterface> provider = REF.field("provider");
    }
}
