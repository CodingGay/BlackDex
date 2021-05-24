package top.niunaijun.blackdex.view.base

import android.content.Intent
import android.os.Bundle
import top.niunaijun.blackdex.view.main.MainActivity

/**
 *
 * @Description:
 * @Author: wukaicheng
 * @CreateDate: 2021/5/24 21:49
 */
class WelcomeActivity : BaseActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val intent = Intent(this, MainActivity::class.java)
        startActivity(intent)
        finish()
    }
}