package mirror.android.content.pm;

import android.os.Parcelable;

import java.util.List;

import mirror.MirrorReflection;

public class ParceledListSliceJBMR2 {
    public static final MirrorReflection REF = MirrorReflection.on("android.content.pm.ParceledListSlice");
    public static MirrorReflection.ConstructorWrapper<Parcelable> constructor = REF.constructor(List.class);
}
