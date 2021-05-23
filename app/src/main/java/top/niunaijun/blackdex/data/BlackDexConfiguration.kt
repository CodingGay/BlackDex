package top.niunaijun.blackdex.data

import android.content.Context
import top.niunaijun.blackbox.app.configuration.ClientConfiguration

/**
 *
 * @Description: 启动配置文件
 * @Author: wukaicheng
 * @CreateDate: 2021/5/23 14:04
 */
class BlackDexConfiguration(private val context: Context) : ClientConfiguration() {
    override fun getHostPackageName(): String {
        return context.packageName
    }


}