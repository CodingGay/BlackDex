package top.niunaijun.blackdex.view.widget

import android.app.Dialog
import android.os.Bundle
import android.util.Log
import android.view.KeyEvent
import android.view.View
import androidx.appcompat.app.AlertDialog
import androidx.fragment.app.DialogFragment
import top.niunaijun.blackdex.R
import top.niunaijun.blackdex.databinding.DialogProgressBinding
import top.niunaijun.blackdex.util.inflate

/**
 *
 * @Description: progress dialog
 * @Author: wukaicheng
 * @CreateDate: 2021/6/7 21:16
 */
class ProgressDialog : DialogFragment() {

    private val TAG = "ProgressDialog"

    private val viewBinding: DialogProgressBinding by inflate()

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val dialog = AlertDialog.Builder(requireContext())
            .setView(viewBinding.root)
            .setTitle(getString(R.string.classes_progress,1,1))
            .setCancelable(false)
            .show()

        dialog.setCanceledOnTouchOutside(false)
        dialog.setOnKeyListener { _, keyCode, _ ->
            return@setOnKeyListener keyCode == KeyEvent.KEYCODE_BACK
        }

        return dialog
    }


    fun setProgress(progress: Int, maxProgress: Int) {
        requireActivity().runOnUiThread {
            if (progress == 0) {
                viewBinding.progress.max = maxProgress
            }
            viewBinding.progress.progress = progress
            dialog?.setTitle(getString(R.string.classes_progress,progress,maxProgress))

        }
    }

}