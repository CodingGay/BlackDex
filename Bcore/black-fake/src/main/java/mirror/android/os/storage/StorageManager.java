package mirror.android.os.storage;

import android.os.storage.StorageVolume;

import mirror.MirrorReflection;

/**
 * Created by Milk on 4/10/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class StorageManager {
    public static final MirrorReflection REF = MirrorReflection.on("android.os.storage.StorageManager");

    public static MirrorReflection.StaticMethodWrapper<StorageVolume[]> getVolumeList = REF.staticMethod("getVolumeList", int.class, int.class);
}
