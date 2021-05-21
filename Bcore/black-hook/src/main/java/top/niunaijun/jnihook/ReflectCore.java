package top.niunaijun.jnihook;

import java.lang.reflect.Field;
import java.lang.reflect.Method;

import top.niunaijun.jnihook.jni.JniHook;

/**
 * Created by Milk on 2021/5/7.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class ReflectCore {

    public static void set(Class<?> clazz) {
        try {
            Field accessFlags = Class.class.getDeclaredField("accessFlags");
            accessFlags.setAccessible(true);
            int o = (int) accessFlags.get(clazz);
            accessFlags.set(clazz, o | 0x0001);
        } catch (Throwable e) {
            e.printStackTrace();
        }
        for (Method declaredMethod : clazz.getDeclaredMethods()) {
            JniHook.setAccessible(clazz, declaredMethod);
        }
        for (Field declaredField : clazz.getDeclaredFields()) {
            JniHook.setAccessible(clazz, declaredField);
        }
        for (Class<?> declaredClass : clazz.getDeclaredClasses()) {
            set(declaredClass);
        }
    }
}
