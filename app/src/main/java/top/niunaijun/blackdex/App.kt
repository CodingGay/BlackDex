package top.niunaijun.blackdex

import android.app.Application
import android.content.Context
import top.niunaijun.blackbox.BlackDexCore
import top.niunaijun.blackbox.app.configuration.ClientConfiguration
import top.niunaijun.blackdex.data.BlackDexConfiguration

/**
 *
 * @Description:
 * @Author: wukaicheng
 * @CreateDate: 2021/5/23 14:00
 */
class App : Application() {

    override fun attachBaseContext(base: Context?) {
        super.attachBaseContext(base)
        BlackDexCore.get().doAttachBaseContext(base,BlackDexConfiguration(base!!))
    }

    override fun onCreate() {
        super.onCreate()
        BlackDexCore.get().doCreate()
    }
}