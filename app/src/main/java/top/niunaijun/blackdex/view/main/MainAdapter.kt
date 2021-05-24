package top.niunaijun.blackdex.view.main

import android.view.ViewGroup
import top.niunaijun.blackdex.data.entity.AppInfo
import top.niunaijun.blackdex.databinding.ItemPackageBinding
import top.niunaijun.blackdex.util.newBindingViewHolder
import top.niunaijun.blackdex.view.base.BaseAdapter

/**
 *
 * @Description: 软件显示界面适配器
 * @Author: wukaicheng
 * @CreateDate: 2021/4/29 21:52
 */

class MainAdapter : BaseAdapter<ItemPackageBinding, AppInfo>() {
    override fun getViewBinding(parent: ViewGroup): ItemPackageBinding {
        return newBindingViewHolder(parent, false)

    }

    override fun initView(binding: ItemPackageBinding, position: Int, data: AppInfo) {
        binding.icon.setImageDrawable(data.icon)
        binding.name.text = data.name
        binding.packageName.text = data.packageName
    }
}