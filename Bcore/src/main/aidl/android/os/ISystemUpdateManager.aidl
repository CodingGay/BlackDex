package android.os;

import android.os.Bundle;
import android.os.PersistableBundle;

interface ISystemUpdateManager {
    Bundle retrieveSystemUpdateInfo();
    void updateSystemUpdateInfo(in PersistableBundle data);
}