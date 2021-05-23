package top.niunaijun.blackdex.util

import top.niunaijun.blackdex.data.DexDumpRepository
import top.niunaijun.blackdex.view.main.MainFactory


/**
 *
 * @Description:
 * @Author: wukaicheng
 * @CreateDate: 2021/4/29 22:38
 */
object InjectionUtil {

    private val dexDumpRepository = DexDumpRepository()


    fun getMainFactory() : MainFactory {
        return MainFactory(dexDumpRepository)
    }

}