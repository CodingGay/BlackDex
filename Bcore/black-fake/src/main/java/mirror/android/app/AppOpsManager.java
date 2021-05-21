package mirror.android.app;

import android.os.IInterface;

import mirror.MirrorReflection;

/**
 * Created by Milk on 5/20/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class AppOpsManager {
    public static final MirrorReflection REF = MirrorReflection.on(android.app.AppOpsManager.class);
    public static MirrorReflection.FieldWrapper<IInterface> mService = REF.field("mService");
}
