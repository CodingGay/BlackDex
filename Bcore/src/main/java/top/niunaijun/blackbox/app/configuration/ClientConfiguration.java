package top.niunaijun.blackbox.app.configuration;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import top.niunaijun.blackbox.BlackBoxCore;
import top.niunaijun.blackbox.utils.FileUtils;

/**
 * Created by Milk on 5/4/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public abstract class ClientConfiguration {
    private File mExternalFilesDir;

    public final void init() {
        mExternalFilesDir = BlackBoxCore.getContext().getExternalCacheDir().getParentFile();
    }

    public abstract String getHostPackageName();

    public String getDexDumpDir() {
        File dump = new File(mExternalFilesDir, "dump");
        FileUtils.mkdirs(dump);
        return dump.getAbsolutePath();
    }
}
