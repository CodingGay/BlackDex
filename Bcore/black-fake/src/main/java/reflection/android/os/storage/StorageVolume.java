package reflection.android.os.storage;

import java.io.File;

import reflection.MirrorReflection;

/**
 * Created by Milk on 4/10/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class StorageVolume {
    public static final MirrorReflection REF = MirrorReflection.on("android.os.storage.StorageVolume");

    public static MirrorReflection.FieldWrapper<File> mPath = REF.field("mPath");
    public static MirrorReflection.FieldWrapper<File> mInternalPath = REF.field("mInternalPath");
}
