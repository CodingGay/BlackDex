package top.niunaijun.blackdex.data

import android.content.Context
import top.niunaijun.blackbox.app.configuration.ClientConfiguration
import top.niunaijun.blackbox.utils.FileUtils
import top.niunaijun.blackbox.utils.compat.BuildCompat
import java.io.File

/**
 *
 * @Description: 启动配置文件
 * @Author: wukaicheng
 * @CreateDate: 2021/5/23 14:04
 */
class BlackDexConfiguration(private val context: Context) : ClientConfiguration() {

    companion object {
        fun getDexDumpDir(context: Context): String {
            return if (BuildCompat.isR()) {
                val dump = File(context.externalCacheDir?.parentFile?.parentFile?.parentFile?.parentFile, "Download/dexDump")
                FileUtils.mkdirs(dump)
                dump.absolutePath
            } else {
                val dump = File(context.externalCacheDir?.parentFile, "dump")
                FileUtils.mkdirs(dump)
                dump.absolutePath
            }
        }
    }

    private val dir = getDexDumpDir(context)

    override fun getHostPackageName(): String {
        return context.packageName
    }

    override fun getDexDumpDir(): String {
        return dir
    }

}