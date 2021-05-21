package android.app;

import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ProviderInfo;
import android.os.Handler;
import android.os.IBinder;
import android.util.ArrayMap;

import java.lang.ref.WeakReference;
import java.util.Map;
import java.util.Objects;

/**
 * Created by Milk on 2021/5/7.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class ActivityThread {
    public H mH = null;
    public AppBindData mBoundApplication;
    public Application mInitialApplication;
    public Instrumentation mInstrumentation;
    public Map<String, WeakReference<?>> mPackages;
    public Map<IBinder, ActivityClientRecord> mActivities;
    public ArrayMap<ProviderKey, Object> mProviderMap;

    static class H extends Handler {
    }

    public static ActivityThread currentActivityThread() {
        throw new RuntimeException();
    }

    public String getProcessName() {
        throw new RuntimeException();
    }

    public Handler getHandler() {
        throw new RuntimeException();
    }

    public ContentProviderHolder installProvider(Context context,
                                                 ContentProviderHolder holder, ProviderInfo info,
                                                 boolean noisy, boolean noReleaseNeeded, boolean stable) {
        throw new RuntimeException();
    }

    static final class AppBindData {
    }

    public static final class ActivityClientRecord {
        public Activity activity;
        public IBinder token;
        public ActivityInfo activityInfo;
        public Intent intent;
    }

    public static final class ProviderKey {
        public final String authority;
        public final int userId;

        public ProviderKey(String authority, int userId) {
            this.authority = authority;
            this.userId = userId;
        }

        @Override
        public boolean equals(Object o) {
            if (o instanceof ProviderKey) {
                final ProviderKey other = (ProviderKey) o;
                return Objects.equals(authority, other.authority) && userId == other.userId;
            }
            return false;
        }

        @Override
        public int hashCode() {
            return ((authority != null) ? authority.hashCode() : 0) ^ userId;
        }
    }
}
