package top.niunaijun.blackdex.view.setting

import android.os.Bundle
import top.niunaijun.blackdex.R
import top.niunaijun.blackdex.databinding.ActivitySettingBinding
import top.niunaijun.blackdex.util.inflate
import top.niunaijun.blackdex.view.base.BaseActivity
import top.niunaijun.blackdex.view.base.PermissionActivity

class SettingActivity : PermissionActivity() {

    private val viewBinding: ActivitySettingBinding by inflate()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(viewBinding.root)
        initToolbar(viewBinding.toolbarLayout.toolbar, R.string.app_setting,true)
        supportFragmentManager.beginTransaction().replace(R.id.fragment,SettingFragment()).commit()
    }

    fun setRequestCallback(callback:((Boolean)->Unit)?){
        this.requestPermissionCallback = callback
        requestStoragePermission()
    }
}