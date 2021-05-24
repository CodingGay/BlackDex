package reflection.android.app.servertransaction;

import android.content.Intent;

import reflection.MirrorReflection;

public class LaunchActivityItem {
    public static final MirrorReflection REF = MirrorReflection.on("android.app.servertransaction.LaunchActivityItem");

    public static MirrorReflection.FieldWrapper<Intent> mIntent = REF.field("mIntent");
}
