package top.niunaijun.blackbox.core.system.os;

import android.os.Process;
import android.os.RemoteException;
import android.os.storage.StorageVolume;

import top.niunaijun.blackbox.core.env.BEnvironment;
import top.niunaijun.blackbox.core.system.ISystemService;
import top.niunaijun.blackbox.core.system.user.BUserHandle;
import top.niunaijun.blackbox.utils.compat.BuildCompat;

/**
 * Created by Milk on 4/10/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class BStorageManagerService extends IBStorageManagerService.Stub implements ISystemService {
    private static final BStorageManagerService sService = new BStorageManagerService();

    public static BStorageManagerService get() {
        return sService;
    }

    public BStorageManagerService() {
    }

    @Override
    public StorageVolume[] getVolumeList(int uid, String packageName, int flags, int userId) throws RemoteException {
        if (reflection.android.os.storage.StorageManager.getVolumeList == null) {
            return null;
        }
        try {
            StorageVolume[] storageVolumes = reflection.android.os.storage.StorageManager.getVolumeList.call(BUserHandle.getUserId(Process.myUid()), 0);
            for (StorageVolume storageVolume : storageVolumes) {
                reflection.android.os.storage.StorageVolume.mPath.set(storageVolume, BEnvironment.getExternalUserDir(userId));
                if (BuildCompat.isPie()) {
                    reflection.android.os.storage.StorageVolume.mInternalPath.set(storageVolume, BEnvironment.getExternalUserDir(userId));
                }
            }
            return storageVolumes;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return new StorageVolume[]{};
    }

    @Override
    public void systemReady() {

    }
}
