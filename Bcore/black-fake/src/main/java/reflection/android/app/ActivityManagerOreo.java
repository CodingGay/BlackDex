package reflection.android.app;

import android.os.IInterface;

import reflection.MirrorReflection;

/**
 * Created by Milk on 5/20/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class ActivityManagerOreo {
    public static final MirrorReflection REF = MirrorReflection.on("android.app.ActivityManager");

    public static MirrorReflection.MethodWrapper<IInterface> getService = REF.method("getService");
    public static MirrorReflection.FieldWrapper<Object> IActivityManagerSingleton = REF.field("IActivityManagerSingleton");
}
