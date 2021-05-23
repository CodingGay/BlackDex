package top.niunaijun.blackdex.data.entity

/**
 *
 * @Description:
 * @Author: wukaicheng
 * @CreateDate: 2021/5/23 14:36
 */
data class DumpInfo(
        val state: Int,
        val msg: String = ""
) {
    companion object {
        const val SUCCESS = 200

        const val FAIL = 404

        const val LOADING = 300

        const val TIMEOUT = 500
    }
}