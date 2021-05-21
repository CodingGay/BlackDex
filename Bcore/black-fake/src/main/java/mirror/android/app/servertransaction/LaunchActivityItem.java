package mirror.android.app.servertransaction;

import android.content.Intent;

import mirror.MirrorReflection;

public class LaunchActivityItem {
    public static final MirrorReflection REF = MirrorReflection.on("android.app.servertransaction.LaunchActivityItem");

    public static MirrorReflection.FieldWrapper<Intent> mIntent = REF.field("mIntent");
}
