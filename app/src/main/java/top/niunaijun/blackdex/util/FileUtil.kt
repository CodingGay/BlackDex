package top.niunaijun.blackdex.util

import java.io.File

/**
 *
 * @Description: file util
 * @Author: wukaicheng
 * @CreateDate: 2021/5/30 20:25
 */
object FileUtil {

    fun filterApk(file: File):Boolean{
        return (file.extension == "apk") or file.isDirectory
    }
}