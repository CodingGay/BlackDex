package reflection.android.providers;


import android.annotation.TargetApi;
import android.os.Build;
import android.os.IInterface;

import reflection.MirrorReflection;

public class Settings {

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
    public static class Global {
        public static final MirrorReflection REF = MirrorReflection.on(android.provider.Settings.Global.class);
        public static MirrorReflection.FieldWrapper<Object> sNameValueCache = REF.field("sNameValueCache");
    }

    public static class NameValueCache {
        public static final MirrorReflection REF = MirrorReflection.on("android.provider.Settings$NameValueCache");
        public static MirrorReflection.FieldWrapper<Object> mContentProvider = REF.field("mContentProvider");
    }

    public static class NameValueCacheOreo {
        public static final MirrorReflection REF = MirrorReflection.on("android.provider.Settings$NameValueCache");
        public static MirrorReflection.FieldWrapper<Object> mProviderHolder = REF.field("mProviderHolder");
    }

    public static class ContentProviderHolder {
        public static final MirrorReflection REF = MirrorReflection.on("android.provider.Settings$ContentProviderHolder");
        public static MirrorReflection.FieldWrapper<IInterface> mContentProvider = REF.field("mContentProvider");
    }

    public static class Secure {
        public static final MirrorReflection REF = MirrorReflection.on(android.provider.Settings.Secure.class);
        public static MirrorReflection.FieldWrapper<Object> sNameValueCache = REF.field("sNameValueCache");
    }

    public static class System {
        public static final MirrorReflection REF = MirrorReflection.on(android.provider.Settings.System.class);
        public static MirrorReflection.FieldWrapper<Object> sNameValueCache = REF.field("sNameValueCache");
    }
}