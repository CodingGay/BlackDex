package top.niunaijun.blackdex.view.base

import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.Toolbar

/**
 *
 * @Description:BaseActivity
 * @Author: wukaicheng
 * @CreateDate: 2021/5/4 15:58
 */
open class BaseActivity : AppCompatActivity() {

    protected fun initToolbar(toolbar: Toolbar,title:Int, showBack: Boolean = false, onBack: (() -> Unit)? = null) {
        setSupportActionBar(toolbar)
        toolbar.setTitle(title)
        if (showBack) {
            supportActionBar?.let {
                it.setDisplayHomeAsUpEnabled(true)
                toolbar.setNavigationOnClickListener {
                    if (onBack != null) {
                        onBack()
                    }else{
                        finish()
                    }
                }
            }
        }
    }
}