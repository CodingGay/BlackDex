package reflection.android.os;

import android.os.Parcel;

import reflection.MirrorReflection;

public class BundleICS {
    public static final MirrorReflection REF = MirrorReflection.on(android.os.Bundle.class);
    public static MirrorReflection.FieldWrapper<Parcel> mParcelledData = REF.field("mParcelledData");
}