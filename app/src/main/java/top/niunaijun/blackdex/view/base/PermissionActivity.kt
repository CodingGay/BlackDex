package top.niunaijun.blackdex.view.base

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Environment
import android.provider.Settings
import androidx.activity.result.contract.ActivityResultContracts
import androidx.annotation.RequiresApi
import com.afollestad.materialdialogs.MaterialDialog
import top.niunaijun.blackbox.utils.compat.BuildCompat
import top.niunaijun.blackdex.R

/**
 *
 * @Description:request permission activity
 * @Author: wukaicheng
 * @CreateDate: 2021/5/30 21:27
 */
open class PermissionActivity:BaseActivity() {

    protected var requestPermissionCallback: ((Boolean) -> Unit)? = null

    protected fun requestStoragePermission() {
        @RequiresApi(Build.VERSION_CODES.R)
        if (BuildCompat.isR()) {
            if (Environment.isExternalStorageManager()) {
                //fuck 请求了读取全部文件权限竟然还要申请普通读写权限
                requestPermissionLauncher.launch(Manifest.permission.WRITE_EXTERNAL_STORAGE)
            } else {
                MaterialDialog(this).show {
                    title(R.string.grant_permission)
                    message(res = R.string.request_storage_msg)
                    negativeButton(res = R.string.request_later) {
                        if(requestPermissionCallback!=null){
                            requestPermissionCallback!!(false)
                        }
                    }
                    positiveButton(res = R.string.jump_grant) {
                        val intent = Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION)
                        intent.data = Uri.fromParts("package", packageName, null)
                        startActivity(intent)
                    }
                }
            }
        } else if (BuildCompat.isM() && checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_DENIED) {
            requestPermissionLauncher.launch(Manifest.permission.WRITE_EXTERNAL_STORAGE)
        }else{
            if(requestPermissionCallback!=null){
                requestPermissionCallback!!(true)
            }
        }
    }


    @RequiresApi(Build.VERSION_CODES.M)
    private val requestPermissionLauncher =
        registerForActivityResult(ActivityResultContracts.RequestPermission()) {
            if (it) {
                if(requestPermissionCallback!=null){
                    requestPermissionCallback!!(true)
                }
            } else {
                MaterialDialog(this).show {
                    title(res = R.string.request_fail)
                    message(res = R.string.denied_msg)
                    if (shouldShowRequestPermissionRationale(Manifest.permission.WRITE_EXTERNAL_STORAGE)) {
                        positiveButton(res = R.string.request_again) {
                            requestStoragePermission()
                        }

                    } else {
                        positiveButton(res = R.string.jump_grant) {
                            val intent = Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS)
                            val uri = Uri.fromParts("package", packageName, null)
                            intent.data = uri
                            startActivity(intent)
                        }
                    }
                    negativeButton(res = R.string.request_later) {
                        if(requestPermissionCallback!=null){
                            requestPermissionCallback!!(false)
                        }
                    }
                }
            }
        }


    override fun onStart() {
        super.onStart()
        if(requestPermissionCallback!=null){
            requestStoragePermission()
        }
    }

}