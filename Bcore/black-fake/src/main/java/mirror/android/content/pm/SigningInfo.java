package mirror.android.content.pm;

import android.content.pm.PackageParser;

import mirror.MirrorReflection;

/**
 * Created by Milk on 4/16/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class SigningInfo {
    public static final MirrorReflection REF = MirrorReflection.on("android.content.pm.SigningInfo");

    public static MirrorReflection.FieldWrapper<PackageParser.SigningDetails> mSigningDetails = REF.field("mSigningDetails");
}
