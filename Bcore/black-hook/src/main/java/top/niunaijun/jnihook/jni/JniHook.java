package top.niunaijun.jnihook.jni;

import java.lang.reflect.Field;
import java.lang.reflect.Method;

/**
 * Created by Milk on 3/7/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public final class JniHook {
    public static final int NATIVE_OFFSET = 0;

    public static final int NATIVE_OFFSET_2 = 0;

    public static final native void nativeOffset();

    public static final native void nativeOffset2();

    public static native void setAccessible(Class<?> clazz, Method method);

    public static native void setAccessible(Class<?> clazz, Field field);
}
