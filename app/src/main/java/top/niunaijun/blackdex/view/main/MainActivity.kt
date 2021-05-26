package top.niunaijun.blackdex.view.main

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.provider.Settings
import android.view.Menu
import android.view.MenuItem
import android.view.inputmethod.InputMethodManager
import androidx.activity.result.contract.ActivityResultContracts
import androidx.annotation.RequiresApi
import androidx.lifecycle.ViewModelProvider
import androidx.recyclerview.widget.LinearLayoutManager
import com.afollestad.materialdialogs.MaterialDialog
import com.ferfalk.simplesearchview.SimpleSearchView
import com.roger.catloadinglibrary.CatLoadingView
import top.niunaijun.blackbox.BlackDexCore
import top.niunaijun.blackbox.core.system.dump.IBDumpMonitor
import top.niunaijun.blackbox.entity.dump.DumpResult
import top.niunaijun.blackbox.utils.compat.BuildCompat
import top.niunaijun.blackdex.R
import top.niunaijun.blackdex.data.entity.AppInfo
import top.niunaijun.blackdex.data.entity.DumpInfo
import top.niunaijun.blackdex.databinding.ActivityMainBinding
import top.niunaijun.blackdex.util.InjectionUtil
import top.niunaijun.blackdex.util.LoadingUtil
import top.niunaijun.blackdex.util.inflate
import top.niunaijun.blackdex.view.base.BaseActivity

class MainActivity : BaseActivity() {

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
            openDocumentedResult.launch("application/vnd.android.package-archive")
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
                            title(text = "脱壳失败")
                            message(text = "未知错误，可前往GitHub(https://github.com/CodingGay/BlackDex)提Issue")
                            negativeButton(text = "Github") {
                                val intent = Intent(
                                    Intent.ACTION_VIEW,
                                    Uri.parse("https://github.com/CodingGay/BlackDex/issues")
                                )
                                startActivity(intent)
                            }
                            positiveButton(text = "确定")

                        }
                    }
                    else -> {
                        viewModel.dexDumpSuccess()
                        val title = if (it.state == DumpInfo.SUCCESS) {
                            "脱壳成功"
                        } else {
                            "脱壳失败"
                        }
                        loadingView.dismiss()
                        MaterialDialog(this).show {
                            title(text = title)
                            message(text = it.msg)
                            positiveButton(text = "确定")
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
                            "DEX文件储存在:${result.dir}"
                        )
                    )
                } else {
                    viewModel.mDexDumpLiveData.postValue(
                        DumpInfo(
                            DumpInfo.FAIL,
                            "错误原因: ${result.msg}"
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

    private fun requestStoragePermission() {
        if (BuildCompat.isM() && checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_DENIED) {
            requestPermissionLauncher.launch(Manifest.permission.WRITE_EXTERNAL_STORAGE)
        }
    }

    private val openDocumentedResult =
        registerForActivityResult(ActivityResultContracts.GetContent()) {
            it?.run {
                viewModel.startDexDump(it.toString())
            }
        }

    @RequiresApi(Build.VERSION_CODES.M)
    private val requestPermissionLauncher =
        registerForActivityResult(ActivityResultContracts.RequestPermission()) {
            if (!it) {
                MaterialDialog(this).show {
                    title(text = "申请失败")
                    message(text = "请授予我们读写本地文件权限，否则软件将无法正常运行。")

                    if (shouldShowRequestPermissionRationale(Manifest.permission.WRITE_EXTERNAL_STORAGE)) {

                        positiveButton(text = "再次申请") {
                            requestStoragePermission()
                        }

                    } else {

                        positiveButton(text = "手动授予") {
                            val intent = Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS)
                            val uri = Uri.fromParts("package", packageName, null)
                            intent.data = uri
                            try {
                                startActivity(intent)
                            } catch (e: Exception) {
                                e.printStackTrace()
                            }
                        }
                    }
                    negativeButton(text = "退出软件") {
                        finish()
                    }
                }
            }
        }


    override fun onStart() {
        super.onStart()
        requestStoragePermission()
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
        }

        return super.onOptionsItemSelected(item)
    }

}