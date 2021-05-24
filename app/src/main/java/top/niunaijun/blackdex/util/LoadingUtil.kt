package top.niunaijun.blackdex.util

import android.view.KeyEvent
import androidx.fragment.app.FragmentManager
import com.roger.catloadinglibrary.CatLoadingView
import top.niunaijun.blackdex.R

/**
 *
 * @Description:
 * @Author: wukaicheng
 * @CreateDate: 2021/4/30 23:04
 */
object LoadingUtil {

    fun showLoading(loadingView: CatLoadingView, fragmentManager: FragmentManager) {
        if (!loadingView.isAdded) {
            loadingView.setBackgroundColor(R.color.primary)
            loadingView.show(fragmentManager, "")
            fragmentManager.executePendingTransactions()
            loadingView.setClickCancelAble(false)
            loadingView.dialog?.setOnKeyListener { _, keyCode, _ ->
                if (keyCode == KeyEvent.KEYCODE_BACK || keyCode == KeyEvent.KEYCODE_ESCAPE) {
                    return@setOnKeyListener true
                }
                false
            }
        }
    }
}