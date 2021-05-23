  package top.niunaijun.blackdex.view.base

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import androidx.viewbinding.ViewBinding

/**
 *
 * @Description: 抽象adapter
 * @Author: wukaicheng
 * @CreateDate: 2021/4/29 21:42
 */
abstract class BaseAdapter<T : ViewBinding, D> : RecyclerView.Adapter<BaseAdapter.ViewHolder<T>>() {

    var dataList: MutableList<D> = ArrayList()

    private var onItemClick: ((position: Int, binding: T, data: D) -> Unit)? = null

    private var onLongClick: ((position: Int, binding: T, data: D) -> Unit)? = null


    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder<T> {
        return ViewHolder(getViewBinding(parent))
    }


    override fun getItemCount(): Int {
        return dataList.size
    }

    override fun onBindViewHolder(holder: ViewHolder<T>, position: Int) {
        val bean = dataList[position]
        val binding = holder.bindIng
        initView(binding, position, bean)

        binding.root.setOnClickListener {
            if (onItemClick != null) {
                onItemClick!!(position, holder.bindIng, bean)
            }
        }
        binding.root.setOnLongClickListener {
            if (onLongClick != null) {
                onLongClick!!(position, holder.bindIng, bean)
            }
            true
        }
    }

    open fun replaceData(newDataList: List<D>) {
        this.dataList = arrayListOf<D>().apply {
            newDataList.forEach {
                this.add(it)
            }
        }
        notifyDataSetChanged()
    }

    open fun addData(list: List<D>) {
        val index = this.dataList.size
        this.dataList.addAll(list)
        notifyItemRangeInserted(index, list.size)
    }

    open fun addData(bean: D) {
        val index = this.dataList.size
        this.dataList.add(bean)
        notifyItemRangeInserted(index, 1)
    }

    open fun updateData(bean: D, position: Int) {
        if (dataList.size > position) {
            dataList[position] = bean
            notifyItemChanged(position)
        }
    }

    open fun removeData(bean: D): Int {
        val position: Int = this.dataList.indexOf(bean)
        if (position >= 0) {
            removeDataAt(position)
        }
        return position
    }

    open fun removeDataAt(position: Int) {
        if (position >= 0) {
            this.dataList.removeAt(position)
            notifyItemRemoved(position)
            notifyItemRangeChanged(position, dataList.size - position)
        }
    }

    fun setOnItemClick(function: (position: Int, binding: T, data: D) -> Unit) {
        this.onItemClick = function

    }

    fun setOnItemLongClick(function: (position: Int, binding: T, data: D) -> Unit) {
        this.onLongClick = function

    }

    fun getLayoutInflater(parent: ViewGroup): LayoutInflater {
        return LayoutInflater.from(parent.context)
    }

    abstract fun getViewBinding(parent: ViewGroup): T

    abstract fun initView(binding: T, position: Int, data: D)


    class ViewHolder<T : ViewBinding>(val bindIng: T) : RecyclerView.ViewHolder(bindIng.root)

}