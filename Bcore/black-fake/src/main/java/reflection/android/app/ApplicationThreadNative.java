package reflection.android.app;

import android.os.IBinder;
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
public class ApplicationThreadNative {
    public static final MirrorReflection REF = MirrorReflection.on("android.app.ApplicationThreadNative");

    public static MirrorReflection.MethodWrapper<IInterface> asInterface = REF.method("asInterface", IBinder.class);
}
