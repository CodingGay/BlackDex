package android.app;

import android.content.IContentProvider;
import android.content.pm.ProviderInfo;
import android.os.IBinder;

/**
 * Created by Milk on 2021/5/7.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class ContentProviderHolder {
    public final ProviderInfo info = null;
    public IContentProvider provider;
    public IBinder connection;
}
