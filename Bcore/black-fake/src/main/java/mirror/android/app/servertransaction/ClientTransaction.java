package mirror.android.app.servertransaction;

import java.util.List;

import mirror.MirrorReflection;

public class ClientTransaction {
    public static final MirrorReflection REF = MirrorReflection.on("android.app.servertransaction.ClientTransaction");

    public static MirrorReflection.FieldWrapper<List<Object>> mActivityCallbacks = REF.field("mActivityCallbacks");
}
