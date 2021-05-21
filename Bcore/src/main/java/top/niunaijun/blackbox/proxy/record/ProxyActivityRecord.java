package top.niunaijun.blackbox.proxy.record;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.os.IBinder;

import top.niunaijun.blackbox.utils.compat.BundleCompat;

/**
 * Created by Milk on 3/31/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class ProxyActivityRecord {
    public int mUserId;
    public ActivityInfo mActivityInfo;
    public Intent mTarget;
    public IBinder mActivityRecord;

    public ProxyActivityRecord(int userId, ActivityInfo activityInfo, Intent target, IBinder activityRecord) {
        mUserId = userId;
        mActivityInfo = activityInfo;
        mTarget = target;
        mActivityRecord = activityRecord;
    }

    public static void saveStub(Intent shadow, Intent target, ActivityInfo activityInfo, IBinder activityRecord, int userId) {
        shadow.putExtra("_VM_|_user_id_", userId);
        shadow.putExtra("_VM_|_activity_info_", activityInfo);
        shadow.putExtra("_VM_|_target_", target);
        BundleCompat.putBinder(shadow, "_VM_|_activity_record_v_", activityRecord);
    }

    public static ProxyActivityRecord create(Intent intent) {
        int userId = intent.getIntExtra("_VM_|_user_id_", 0);
        ActivityInfo activityInfo = intent.getParcelableExtra("_VM_|_activity_info_");
        Intent target = intent.getParcelableExtra("_VM_|_target_");
        IBinder activityRecord = BundleCompat.getBinder(intent, "_VM_|_activity_record_v_");
        return new ProxyActivityRecord(userId, activityInfo, target, activityRecord);
    }
}
