package top.niunaijun.blackdex.data

import android.content.pm.ApplicationInfo
import android.net.Uri
import android.webkit.URLUtil
import androidx.lifecycle.MutableLiveData
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import top.niunaijun.blackbox.BlackBoxCore
import top.niunaijun.blackbox.BlackBoxCore.getPackageManager
import top.niunaijun.blackbox.BlackDexCore
import top.niunaijun.blackbox.utils.AbiUtils
import top.niunaijun.blackbox.utils.FileUtils
import top.niunaijun.blackdex.app.App
import top.niunaijun.blackdex.app.AppManager
import top.niunaijun.blackdex.app.BlackDexLoader
import top.niunaijun.blackdex.data.entity.AppInfo
import top.niunaijun.blackdex.data.entity.DumpInfo
import java.io.File

/**
 *
 * @Description:
 * @Author: wukaicheng
 * @CreateDate: 2021/5/23 14:29
 */
class DexDumpRepository {

    private var dumpTaskId = 0

    fun getAppList(mAppListLiveData: MutableLiveData<List<AppInfo>>) {

        val installedApplications: List<ApplicationInfo> =
                getPackageManager().getInstalledApplications(0)
        val installedList = mutableListOf<AppInfo>()

        for (installedApplication in installedApplications) {
            val file = File(installedApplication.sourceDir)

            if ((installedApplication.flags and ApplicationInfo.FLAG_SYSTEM) != 0) continue

            if (!AbiUtils.isSupport(file)) continue


            val info = AppInfo(
                    installedApplication.loadLabel(getPackageManager()).toString(),
                    installedApplication.packageName,
                    installedApplication.loadIcon(getPackageManager())
            )
            installedList.add(info)
        }

        mAppListLiveData.postValue(installedList)
    }

    fun dumpDex(source: String, dexDumpLiveData: MutableLiveData<DumpInfo>) {

        dexDumpLiveData.postValue(DumpInfo(DumpInfo.LOADING))

        val result = if (URLUtil.isValidUrl(source)) {
            BlackDexCore.get().dumpDex(Uri.parse(source))
        } else if (source.contains("/")) {
            BlackDexCore.get().dumpDex(File(source))
        } else {
            BlackDexCore.get().dumpDex(source)
        }

        if (result) {
            dumpTaskId++
            startCountdown(dexDumpLiveData)
        } else {
            dexDumpLiveData.postValue(DumpInfo(DumpInfo.TIMEOUT))
        }

    }


    fun dumpSuccess() {
        dumpTaskId++
    }

    private fun startCountdown(dexDumpLiveData: MutableLiveData<DumpInfo>) {
        GlobalScope.launch {
            val tempId = dumpTaskId
            while (BlackDexCore.get().isRunning) {
                delay(10000)
                //10s
                if (!AppManager.mBlackBoxLoader.isFixCodeItem()) {
                    break
                }
                //fixCodeItem 需要长时间运行，普通内存dump不需要
            }
            if (tempId == dumpTaskId) {
                dexDumpLiveData.postValue(DumpInfo(DumpInfo.TIMEOUT))
            }
        }
    }
}