package top.niunaijun.blackdex.app

import android.content.Context
import top.niunaijun.blackbox.BlackDexCore
import top.niunaijun.blackbox.app.configuration.ClientConfiguration
import top.niunaijun.blackbox.utils.FileUtils
import top.niunaijun.blackbox.utils.compat.BuildCompat
import top.niunaijun.blackboxa.app.App
import top.niunaijun.blackdex.biz.cache.AppSharedPreferenceDelegate
import java.io.File

/**
 *
 * @Description:
 * @Author: wukaicheng
 * @CreateDate: 2021/5/6 23:38
 */
class BlackDexLoader {


    private var mSavePath by AppSharedPreferenceDelegate(App.getContext(), "")

    private var mSaveEnable by AppSharedPreferenceDelegate(App.getContext(), false)

    private var mDir = if (mSaveEnable) {
        getDexDumpDir(App.getContext())
    } else {
        mSavePath
    }

    fun addLifecycleCallback() {

    }

    fun attachBaseContext(context: Context) {
        BlackDexCore.get().doAttachBaseContext(context, object : ClientConfiguration() {
            override fun getHostPackageName(): String {
                return context.packageName
            }

            override fun getDexDumpDir(): String {
                return mDir
            }
        })
    }

    fun doOnCreate(context: Context) {
        BlackDexCore.get().doCreate()
    }

    fun saveEnable(): Boolean {
        return mSaveEnable
    }

    fun saveEnable(state: Boolean) {
        this.mSaveEnable = state
    }

    fun getSavePath(): String {
        return mSavePath
    }

    fun setSavePath(path: String) {
        this.mSavePath = path
    }


    companion object {

        val TAG: String = BlackDexLoader::class.java.simpleName

        fun getDexDumpDir(context: Context): String {
            return if (BuildCompat.isR()) {
                val dump = File(
                    context.externalCacheDir?.parentFile?.parentFile?.parentFile?.parentFile,
                    "hupu/dexDump"
                )
                FileUtils.mkdirs(dump)
                dump.absolutePath
            } else {
                val dump = File(context.externalCacheDir?.parentFile, "dump")
                FileUtils.mkdirs(dump)
                dump.absolutePath
            }
        }
    }
}