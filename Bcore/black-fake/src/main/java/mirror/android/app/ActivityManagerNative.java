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
public class ActivityManagerNative {
    public static final MirrorReflection REF = MirrorReflection.on("android.app.ActivityManagerNative");

    public static MirrorReflection.FieldWrapper<Object> gDefault = REF.field("gDefault");
    public static MirrorReflection.StaticMethodWrapper<IInterface> getDefault = REF.staticMethod("getDefault");
}
