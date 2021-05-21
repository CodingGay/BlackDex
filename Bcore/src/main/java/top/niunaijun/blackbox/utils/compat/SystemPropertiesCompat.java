package top.niunaijun.blackbox.utils.compat;

import android.text.TextUtils;

import top.niunaijun.blackbox.utils.Reflector;


public class SystemPropertiesCompat {

    public static String get(String key, String def) {
        try {
            return (String) Reflector.on("android.os.SystemProperties")
                    .method("get", String.class, String.class)
                    .call(key, def);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return def;
    }

    public static String get(String key) {
        try {
            return (String) Reflector.on("android.os.SystemProperties")
                    .method("get", String.class)
                    .call(key);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    public static boolean isExist(String key) {
        return !TextUtils.isEmpty(get(key));
    }

    public static int getInt(String key, int def) {
        try {
            return (int) Reflector.on("android.os.SystemProperties")
                    .method("getInt", String.class, int.class)
                    .call(key, def);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return def;
    }

}
