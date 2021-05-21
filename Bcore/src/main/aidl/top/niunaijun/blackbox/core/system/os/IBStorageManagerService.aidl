// IBStorageManagerService.aidl
package top.niunaijun.blackbox.core.system.os;

import android.os.storage.StorageVolume;
import java.lang.String;
import android.net.Uri;

// Declare any non-default types here with import statements

interface IBStorageManagerService {
      StorageVolume[] getVolumeList(int uid, String packageName, int flags, int userId);
}
