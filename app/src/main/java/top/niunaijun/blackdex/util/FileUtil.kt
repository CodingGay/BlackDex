package top.niunaijun.blackdex.util

import top.niunaijun.blackbox.BlackBoxCore
import top.niunaijun.blackbox.utils.FileUtils
import top.niunaijun.blackbox.utils.compat.BuildCompat
import java.io.File

/**
 *
 * @Description:
 * @Author: wukaicheng
 * @CreateDate: 2021/5/23 14:54
 */
object FileUtil {

    fun getDexDumpDir(): String {
        if (BuildCompat.isR()) {
            val dump = File(BlackBoxCore.getContext().externalCacheDir?.parentFile?.parentFile?.parentFile, "Download/dexDump")
            FileUtils.mkdirs(dump)
            return dump.absolutePath
        }else{
            val dump = File(BlackBoxCore.getContext().externalCacheDir?.parentFile, "dump")
            FileUtils.mkdirs(dump)
            return dump.absolutePath
        }
    }
}