package top.niunaijun.blackbox.utils;

import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ComponentInfo;
import android.content.pm.ProviderInfo;

import java.util.Objects;

import top.niunaijun.blackbox.app.BActivityThread;

import static android.content.pm.ActivityInfo.LAUNCH_SINGLE_INSTANCE;

public class ComponentUtils {

    public static boolean isRequestInstall(Intent intent) {
        return "application/vnd.android.package-archive".equals(intent.getType());
    }

    public static boolean isSelf(Intent intent) {
        ComponentName component = intent.getComponent();
        if (component == null || BActivityThread.getAppPackageName() == null) return false;
        return component.getPackageName().equals(BActivityThread.getAppPackageName());
    }

    public static boolean isSelf(Intent[] intent) {
        for (Intent intent1 : intent) {
            if (!isSelf(intent1)) {
                return false;
            }
        }
        return true;
    }

    public static boolean isGmsService(Intent intent) {
        String aPackage = intent.getPackage();
        return "com.google.android.gms".equals(aPackage);
    }

    public static String getTaskAffinity(ActivityInfo info) {
        if (info.launchMode == LAUNCH_SINGLE_INSTANCE) {
            return "-SingleInstance-" + info.packageName + "/" + info.name;
        } else if (info.taskAffinity == null && info.applicationInfo.taskAffinity == null) {
            return info.packageName;
        } else if (info.taskAffinity != null) {
            return info.taskAffinity;
        }
        return info.applicationInfo.taskAffinity;
    }

    public static String getFirstAuthority(ProviderInfo info) {
        if (info == null) {
            return null;
        }
        String[] authorities = info.authority.split(";");
        return authorities.length == 0 ? info.authority : authorities[0];
    }

    public static boolean intentFilterEquals(Intent a, Intent b) {
        if (a != null && b != null) {
            if (!Objects.equals(a.getAction(), b.getAction())) {
                return false;
            }
            if (!Objects.equals(a.getData(), b.getData())) {
                return false;
            }
            if (!Objects.equals(a.getType(), b.getType())) {
                return false;
            }
            Object pkgA = a.getPackage();
            if (pkgA == null && a.getComponent() != null) {
                pkgA = a.getComponent().getPackageName();
            }
            String pkgB = b.getPackage();
            if (pkgB == null && b.getComponent() != null) {
                pkgB = b.getComponent().getPackageName();
            }
            if (!Objects.equals(pkgA, pkgB)) {
                return false;
            }
            if (!Objects.equals(a.getComponent(), b.getComponent())) {
                return false;
            }
            if (!Objects.equals(a.getCategories(), b.getCategories())) {
                return false;
            }
        }
        return true;
    }

    public static String getProcessName(ComponentInfo componentInfo) {
        String processName = componentInfo.processName;
        if (processName == null) {
            processName = componentInfo.packageName;
            componentInfo.processName = processName;
        }
        return processName;
    }

    public static boolean isSameComponent(ComponentInfo first, ComponentInfo second) {

        if (first != null && second != null) {
            String pkg1 = first.packageName + "";
            String pkg2 = second.packageName + "";
            String name1 = first.name + "";
            String name2 = second.name + "";
            return pkg1.equals(pkg2) && name1.equals(name2);
        }
        return false;
    }

    public static ComponentName toComponentName(ComponentInfo componentInfo) {
        return new ComponentName(componentInfo.packageName, componentInfo.name);
    }
}
