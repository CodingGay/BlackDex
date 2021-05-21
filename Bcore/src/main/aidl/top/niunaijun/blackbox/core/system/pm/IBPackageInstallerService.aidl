// IBPackageInstallerService.aidl
package top.niunaijun.blackbox.core.system.pm;

import top.niunaijun.blackbox.core.system.pm.BPackageSettings;
import top.niunaijun.blackbox.entity.pm.InstallOption;

// Declare any non-default types here with import statements

interface IBPackageInstallerService {
    int installPackageAsUser(in BPackageSettings file, int userId);
    int uninstallPackageAsUser(in BPackageSettings file, boolean removeApp, int userId);
    int updatePackage(in BPackageSettings file);
}
