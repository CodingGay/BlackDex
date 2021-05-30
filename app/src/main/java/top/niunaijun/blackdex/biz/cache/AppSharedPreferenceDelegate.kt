package top.niunaijun.blackdex.biz.cache


import android.content.Context
import android.text.TextUtils
import androidx.core.content.edit
import kotlin.properties.ReadWriteProperty
import kotlin.reflect.KProperty


/**
 *
 * @desc:目前只支持 5种基本数据类型，如果要支持obj，请继承该类并重写他的相关方法 findData/putData
 *
 * @author: mini
 * @created by 2021/5/10
 */
open class AppSharedPreferenceDelegate<Data>(context: Context, private val default: Data, spName: String? = null) : ReadWriteProperty<Any, Data?> {

    private val mSharedPreferences by lazy {
        val tmpCacheName = if (TextUtils.isEmpty(spName)) {
            AppSharedPreferenceDelegate::class.java.simpleName
        } else {
            spName
        }
        return@lazy context.getSharedPreferences(tmpCacheName, Context.MODE_PRIVATE)
    }

    override fun getValue(thisRef: Any, property: KProperty<*>): Data {
        return findData(property.name, default)
    }

    override fun setValue(thisRef: Any, property: KProperty<*>, value: Data?) {
        putData(property.name, value)
    }

    protected fun findData(key: String, default: Data): Data {
        with(mSharedPreferences) {
            val result: Any? = when (default) {
                is Int -> getInt(key, default)
                is Long -> getLong(key, default)
                is Float -> getFloat(key, default)
                is String -> getString(key, default)
                is Boolean -> getBoolean(key, default)
                else -> throw IllegalArgumentException("This type $default can not be saved into sharedPreferences")
            }
            return result as? Data ?: default
        }
    }

    protected fun putData(key: String, value: Data?) {
        mSharedPreferences.edit {
            if (value == null) {
                remove(key)
            } else {
                when (value) {
                    is Int -> putInt(key, value)
                    is Long -> putLong(key, value)
                    is Float -> putFloat(key, value)
                    is String -> putString(key, value)
                    is Boolean -> putBoolean(key, value)
                    else -> throw IllegalArgumentException("This type $default can not be saved into Preferences")
                }
            }
        }
    }
}