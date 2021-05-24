package top.niunaijun.blackbox.fake.delegate;

import android.net.Uri;
import android.os.Build;
import android.os.IInterface;

import java.lang.reflect.Proxy;
import java.util.HashSet;
import java.util.Set;

import reflection.android.app.IActivityManager;
import reflection.android.content.ContentProviderHolderOreo;
import reflection.android.providers.Settings;
import top.niunaijun.blackbox.BlackBoxCore;
import top.niunaijun.blackbox.fake.service.provider.ContentProviderStub;
import top.niunaijun.blackbox.fake.service.provider.SettingsProviderStub;
import top.niunaijun.blackbox.utils.compat.BuildCompat;

/**
 * Created by Milk on 3/31/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class ContentProviderDelegate {
    public static final String TAG = "ContentProviderDelegate";
    private static Set<String> sInjected = new HashSet<>();

    public static void update(Object holder, String auth) {
        IInterface iInterface;
        if (BuildCompat.isOreo()) {
            iInterface = ContentProviderHolderOreo.provider.get(holder);
        } else {
            iInterface = IActivityManager.ContentProviderHolder.provider.get(holder);
        }

        if (iInterface instanceof Proxy)
            return;
        IInterface vContentProvider;
        switch (auth) {
            case "settings":
                vContentProvider = new SettingsProviderStub().wrapper(iInterface, BlackBoxCore.getHostPkg());
                break;
            default:
                vContentProvider = new ContentProviderStub().wrapper(iInterface, BlackBoxCore.getHostPkg());
                break;
        }
        if (BuildCompat.isOreo()) {
            ContentProviderHolderOreo.provider.set(holder, vContentProvider);
        } else {
            IActivityManager.ContentProviderHolder.provider.set(holder, vContentProvider);
        }
    }

    public static void init() {
        clearSettingProvider();

        BlackBoxCore.getContext().getContentResolver().call(Uri.parse("content://settings"), "", null, null);
    }

    public static void clearSettingProvider() {
        Object cache;
        cache = Settings.System.sNameValueCache.get();
        if (cache != null) {
            clearContentProvider(cache);
        }
        cache = Settings.Secure.sNameValueCache.get();
        if (cache != null) {
            clearContentProvider(cache);
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1 && Settings.Global.REF.getClazz() != null) {
            cache = Settings.Global.sNameValueCache.get();
            if (cache != null) {
                clearContentProvider(cache);
            }
        }
    }

    private static void clearContentProvider(Object cache) {
        if (BuildCompat.isOreo()) {
            Object holder = Settings.NameValueCacheOreo.mProviderHolder.get(cache);
            if (holder != null) {
                Settings.ContentProviderHolder.mContentProvider.set(holder, null);
            }
        } else {
            Settings.NameValueCache.mContentProvider.set(cache, null);
        }
    }
}
