package reflection.android.os;

import android.os.Parcel;

import reflection.MirrorReflection;

public class BaseBundle {
    public static final MirrorReflection REF = MirrorReflection.on("android.os.BaseBundle");
    public static MirrorReflection.FieldWrapper<Parcel> mParcelledData = REF.field("mParcelledData");
}