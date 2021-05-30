package top.niunaijun.blackdex.view.main

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.os.Environment
import android.view.Menu
import android.view.MenuItem
import android.view.inputmethod.InputMethodManager
import androidx.lifecycle.ViewModelProvider
import androidx.recyclerview.widget.LinearLayoutManager
import com.afollestad.materialdialogs.MaterialDialog
import com.afollestad.materialdialogs.files.fileChooser
import com.ferfalk.simplesearchview.SimpleSearchView
import com.roger.catloadinglibrary.CatLoadingView
import com.umeng.analytics.MobclickAgent
import top.niunaijun.blackbox.BlackDexCore
import top.niunaijun.blackbox.core.system.dump.IBDumpMonitor
import top.niunaijun.blackbox.entity.dump.DumpResult
import top.niunaijun.blackdex.R
import top.niunaijun.blackdex.data.entity.AppInfo
import top.niunaijun.blackdex.data.entity.DumpInfo
import top.niunaijun.blackdex.databinding.ActivityMainBinding
import top.niunaijun.blackdex.util.FileUtil
import top.niunaijun.blackdex.util.InjectionUtil
import top.niunaijun.blackdex.util.LoadingUtil
import top.niunaijun.blackdex.util.inflate
import top.niunaijun.blackdex.view.base.PermissionActivity
import top.niunaijun.blackdex.view.setting.SettingActivity


class MainActivity : PermissionActivity() {

    private val viewBinding: ActivityMainBinding by inflate()

    private lateinit var viewModel: MainViewModel

    private lateinit var mAdapter: MainAdapter

    private lateinit var loadingView: CatLoadingView

    private var appList: List<AppInfo> = ArrayList()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(viewBinding.root)

        initToolbar(viewBinding.toolbarLayout.toolbar, R.string.app_name)
        initView()
        initViewModel()
        initSearchView()

        BlackDexCore.get().registerDumpMonitor(mMonitor)

    }

    private fun initView() {
        mAdapter = MainAdapter()
        viewBinding.recyclerView.adapter = mAdapter
        viewBinding.recyclerView.layoutManager = LinearLayoutManager(this)

        mAdapter.setOnItemClick { _, _, data ->
            if (viewBinding.searchView.isSearchOpen) {
                viewBinding.searchView.closeSearch()
                filterApp("")
            }
            hideKeyboard()
            viewModel.startDexDump(data.packageName)
        }

        viewBinding.fab.setOnClickListener {
            this.requestPermissionCallback = {
                if (it) {
                    this.requestPermissionCallback = null
                    val initialDir = Environment.getExternalStorageDirectory()
                    MaterialDialog(this).show {
                        fileChooser(
                            this@MainActivity,
                            initialDirectory = initialDir,
                            filter = FileUtil::filterApk,
                        ) { _, file ->
                            viewModel.startDexDump(file.absolutePath)
                        }

                        negativeButton(res = R.string.cancel)
                    }

                }
            }
            requestStoragePermission()
        }
    }

    private fun initViewModel() {
        viewModel =
            ViewModelProvider(this, InjectionUtil.getMainFactory()).get(MainViewModel::class.java)
        viewBinding.stateView.showLoading()
        viewModel.getAppList()

        viewModel.mAppListLiveData.observe(this) {
            it?.let {
                this.appList = it
                viewBinding.searchView.setQuery("", false)
                filterApp("")
                if (it.isNotEmpty()) {
                    viewBinding.stateView.showContent()
                } else {
                    viewBinding.stateView.showEmpty()
                }
            }
        }

        viewModel.mDexDumpLiveData.observe(this) {
            it?.let {
                when (it.state) {
                    DumpInfo.LOADING -> {
                        showLoading()
                    }
                    DumpInfo.TIMEOUT -> {
                        loadingView.dismiss()
                        MaterialDialog(this).show {
                            title(res = R.string.unpack_fail)
                            message(res = R.string.jump_issue)
                            negativeButton(res = R.string.github) {
                                val intent = Intent(
                                    Intent.ACTION_VIEW,
                                    Uri.parse("https://github.com/CodingGay/BlackDex/issues")
                                )
                                startActivity(intent)
                            }
                            positiveButton(res = R.string.confirm)

                        }
                    }
                    else -> {
                        viewModel.dexDumpSuccess()
                        val title = if (it.state == DumpInfo.SUCCESS) {
                            getString(R.string.unpack_success)
                        } else {
                            getString(R.string.unpack_fail)
                        }
                        loadingView.dismiss()
                        MaterialDialog(this).show {
                            title(text = title)
                            message(text = it.msg)
                            positiveButton(res = R.string.confirm)
                        }
                    }
                }

            }
        }
    }

    private val mMonitor = object : IBDumpMonitor.Stub() {
        override fun onDump(result: DumpResult?) {
            result?.let {
                if (result.success) {
                    viewModel.mDexDumpLiveData.postValue(
                        DumpInfo(
                            DumpInfo.SUCCESS,
                            getString(R.string.dex_save, result.dir)
                        )
                    )
                } else {
                    viewModel.mDexDumpLiveData.postValue(
                        DumpInfo(
                            DumpInfo.FAIL,
                            getString(R.string.error_msg, result.msg)
                        )
                    )
                }
            }
        }

    }


    private fun initSearchView() {
        viewBinding.searchView.setOnQueryTextListener(object :
            SimpleSearchView.OnQueryTextListener {
            override fun onQueryTextChange(newText: String): Boolean {
                filterApp(newText)
                return true
            }

            override fun onQueryTextCleared(): Boolean {
                return true
            }

            override fun onQueryTextSubmit(query: String): Boolean {
                return true
            }

        })
    }

    private fun filterApp(newText: String) {
        val newList = this.appList.filter {
            it.name.contains(newText, true) or it.packageName.contains(newText, true)
        }
        mAdapter.replaceData(newList)
    }

    private fun showLoading() {
        if (!this::loadingView.isInitialized) {
            loadingView = CatLoadingView()
        }

        LoadingUtil.showLoading(loadingView, supportFragmentManager)
    }

    private fun hideKeyboard() {
        val imm: InputMethodManager = getSystemService(INPUT_METHOD_SERVICE) as InputMethodManager
        window.peekDecorView()?.run {
            imm.hideSoftInputFromWindow(windowToken, 0)
        }
    }

    override fun onBackPressed() {
        if (viewBinding.searchView.isSearchOpen) {
            viewBinding.searchView.closeSearch()
            filterApp("")
        } else {
            super.onBackPressed()
        }
    }


    override fun onCreateOptionsMenu(menu: Menu?): Boolean {
        menuInflater.inflate(R.menu.menu_main, menu)
        val item = menu!!.findItem(R.id.main_search)
        viewBinding.searchView.setMenuItem(item)

        return true
    }


    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        when (item.itemId) {
            R.id.main_git -> {
                val intent =
                    Intent(Intent.ACTION_VIEW, Uri.parse("https://github.com/CodingGay/BlackDex"))
                startActivity(intent)
            }

            R.id.main_setting -> {
                val intent =
                    Intent(this, SettingActivity::class.java)
                startActivity(intent)
            }
        }

        return super.onOptionsItemSelected(item)
    }

    override fun onResume() {
        super.onResume()
        MobclickAgent.onResume(this)
    }

    override fun onPause() {
        super.onPause()
        MobclickAgent.onPause(this)
    }
}