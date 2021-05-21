package top.niunaijun.blackbox.core.system.pm;

import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.ProviderInfo;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.util.ArrayMap;

import java.util.ArrayList;
import java.util.List;

import top.niunaijun.blackbox.utils.Slog;


/**
 * Created by Milk on 4/14/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class ComponentResolver {
    public static final String TAG = "ComponentResolver";

    private final Object mLock = new Object();

    /**
     * All available activities, for your resolving pleasure.
     */
    private final ActivityIntentResolver mActivities = new ActivityIntentResolver();

    /**
     * All available providers, for your resolving pleasure.
     */
    private final ProviderIntentResolver mProviders = new ProviderIntentResolver();

    /**
     * All available receivers, for your resolving pleasure.
     */
    private final ActivityIntentResolver mReceivers = new ActivityIntentResolver();

    /**
     * All available services, for your resolving pleasure.
     */
    private final ServiceIntentResolver mServices = new ServiceIntentResolver();
    /**
     * Mapping from provider authority [first directory in content URI codePath) to provider.
     */
    private final ArrayMap<String, BPackage.Provider> mProvidersByAuthority = new ArrayMap<>();

    public ComponentResolver() {
    }

    void addAllComponents(BPackage pkg) {
        final ArrayList<BPackage.ActivityIntentInfo> newIntents = new ArrayList<>();
        synchronized (mLock) {
            addActivitiesLocked(pkg, newIntents);
            addServicesLocked(pkg);
            addProvidersLocked(pkg);
            addReceiversLocked(pkg);
        }
    }

    void removeAllComponents(BPackage pkg) {
        synchronized (mLock) {
            removeAllComponentsLocked(pkg);
        }
    }

    private void removeAllComponentsLocked(BPackage pkg) {
        int componentSize;
        StringBuilder r;
        int i;

        componentSize = pkg.activities.size();
        r = null;
        for (i = 0; i < componentSize; i++) {
            BPackage.Activity a = pkg.activities.get(i);
            mActivities.removeActivity(a, "activity");
        }
        componentSize = pkg.providers.size();
        r = null;
        for (i = 0; i < componentSize; i++) {
            BPackage.Provider p = pkg.providers.get(i);
            mProviders.removeProvider(p);
            if (p.info.authority == null) {
                // Another content provider with this authority existed when this app was
                // installed, so this authority is null. Ignore it as we don't have to
                // unregister the provider.
                continue;
            }
            String[] names = p.info.authority.split(";");
            for (int j = 0; j < names.length; j++) {
                if (mProvidersByAuthority.get(names[j]) == p) {
                    mProvidersByAuthority.remove(names[j]);
                }
            }
        }

        componentSize = pkg.receivers.size();
        r = null;
        for (i = 0; i < componentSize; i++) {
            BPackage.Activity a = pkg.receivers.get(i);
            mReceivers.removeActivity(a, "receiver");
        }

        componentSize = pkg.services.size();
        r = null;
        for (i = 0; i < componentSize; i++) {
            BPackage.Service s = pkg.services.get(i);
            mServices.removeService(s);
        }
    }

    private void addActivitiesLocked(BPackage pkg,
                                     List<BPackage.ActivityIntentInfo> newIntents) {
        final int activitiesSize = pkg.activities.size();
        for (int i = 0; i < activitiesSize; i++) {
            BPackage.Activity a = pkg.activities.get(i);
            a.info.processName =
                    BPackageManagerService.fixProcessName(pkg.applicationInfo.processName, a.info.processName);
            mActivities.addActivity(a, "activity", newIntents);
        }
    }

    private void addProvidersLocked(BPackage pkg) {
        final int providersSize = pkg.providers.size();
        for (int i = 0; i < providersSize; i++) {
            BPackage.Provider p = pkg.providers.get(i);
            p.info.processName = BPackageManagerService.fixProcessName(pkg.applicationInfo.processName,
                    p.info.processName);
            mProviders.addProvider(p);
            if (p.info.authority != null) {
                String[] names = p.info.authority.split(";");
                p.info.authority = null;
                for (String name : names) {
                    if (!mProvidersByAuthority.containsKey(name)) {
                        mProvidersByAuthority.put(name, p);
                        if (p.info.authority == null) {
                            p.info.authority = name;
                        } else {
                            p.info.authority = p.info.authority + ";" + name;
                        }
                    } else {
                        final BPackage.Provider other =
                                mProvidersByAuthority.get(name);
                        final ComponentName component =
                                (other != null && other.getComponentName() != null)
                                        ? other.getComponentName() : null;
                        final String packageName =
                                component != null ? component.getPackageName() : "?";
                        Slog.w(TAG, "Skipping provider name " + name
                                + " (in package " + pkg.applicationInfo.packageName + ")"
                                + ": name already used by " + packageName);
                    }
                }
            }
        }
    }

    private void addReceiversLocked(BPackage pkg) {
        final int receiversSize = pkg.receivers.size();
        for (int i = 0; i < receiversSize; i++) {
            BPackage.Activity a = pkg.receivers.get(i);
            a.info.processName = BPackageManagerService.fixProcessName(pkg.applicationInfo.processName,
                    a.info.processName);
            mReceivers.addActivity(a, "receiver", null);
        }
    }

    private void addServicesLocked(BPackage pkg) {
        final int servicesSize = pkg.services.size();
        for (int i = 0; i < servicesSize; i++) {
            BPackage.Service s = pkg.services.get(i);
            s.info.processName = BPackageManagerService.fixProcessName(pkg.applicationInfo.processName,
                    s.info.processName);
            mServices.addService(s);
        }
    }


    /**
     * Returns the given activity
     */
    BPackage.Activity getActivity(ComponentName component) {
        synchronized (mLock) {
            return mActivities.mActivities.get(component);
        }
    }

    /**
     * Returns the given provider
     */
    BPackage.Provider getProvider(ComponentName component) {
        synchronized (mLock) {
            return mProviders.mProviders.get(component);
        }
    }

    /**
     * Returns the given receiver
     */
    BPackage.Activity getReceiver(ComponentName component) {
        synchronized (mLock) {
            return mReceivers.mActivities.get(component);
        }
    }

    /**
     * Returns the given service
     */
    BPackage.Service getService(ComponentName component) {
        synchronized (mLock) {
            return mServices.mServices.get(component);
        }
    }

    List<ResolveInfo> queryActivities(Intent intent, String resolvedType, int flags, int userId) {
        synchronized (mLock) {
            return mActivities.queryIntent(intent, resolvedType, flags, userId);
        }
    }

    List<ResolveInfo> queryActivities(Intent intent, String resolvedType, int flags,
                                      List<BPackage.Activity> activities, int userId) {
        synchronized (mLock) {
            return mActivities.queryIntentForPackage(
                    intent, resolvedType, flags, activities, userId);
        }
    }

    List<ResolveInfo> queryProviders(Intent intent, String resolvedType, int flags, int userId) {
        synchronized (mLock) {
            return mProviders.queryIntent(intent, resolvedType, flags, userId);
        }
    }

    List<ResolveInfo> queryProviders(Intent intent, String resolvedType, int flags,
                                     List<BPackage.Provider> providers, int userId) {
        synchronized (mLock) {
            return mProviders.queryIntentForPackage(intent, resolvedType, flags, providers, userId);
        }
    }

    List<ProviderInfo> queryProviders(String processName, String metaDataKey, int flags,
                                      int userId) {
        List<ProviderInfo> providerList = new ArrayList<>();
        synchronized (mLock) {
            for (int i = mProviders.mProviders.size() - 1; i >= 0; --i) {
                final BPackage.Provider p = mProviders.mProviders.valueAt(i);
                final BPackageSettings ps = (BPackageSettings) p.owner.mExtras;
                if (ps == null) {
                    continue;
                }
                if (p.info.authority == null) {
                    continue;
                }
                if (processName != null && (!p.info.processName.equals(processName))) {
                    continue;
                }
                // See PM.queryContentProviders()'s javadoc for why we have the metaData parameter.
                if (metaDataKey != null
                        && (p.metaData == null || !p.metaData.containsKey(metaDataKey))) {
                    continue;
                }
                final ProviderInfo info = PackageManagerCompat.generateProviderInfo(p, flags, ps.readUserState(userId), userId);
                if (info == null) {
                    continue;
                }
//                if (providerList == null) {
//                    providerList = new ArrayList<>(i + 1);
//                }
                providerList.add(info);
            }
        }
        return providerList;
    }

    ProviderInfo queryProvider(String authority, int flags, int userId) {
        synchronized (mLock) {
            final BPackage.Provider p = mProvidersByAuthority.get(authority);
            if (p == null) {
                return null;
            }
            BPackageSettings ps = p.owner.mExtras;
            return PackageManagerCompat.generateProviderInfo(p, flags, ps.readUserState(userId), userId);
        }
    }

    List<ResolveInfo> queryReceivers(Intent intent, String resolvedType, int flags, int userId) {
        synchronized (mLock) {
            return mReceivers.queryIntent(intent, resolvedType, flags, userId);
        }
    }

    List<ResolveInfo> queryReceivers(Intent intent, String resolvedType, int flags,
                                     List<BPackage.Activity> receivers, int userId) {
        synchronized (mLock) {
            return mReceivers.queryIntentForPackage(intent, resolvedType, flags, receivers, userId);
        }
    }

    List<ResolveInfo> queryServices(Intent intent, String resolvedType, int flags, int userId) {
        synchronized (mLock) {
            return mServices.queryIntent(intent, resolvedType, flags, userId);
        }
    }

    List<ResolveInfo> queryServices(Intent intent, String resolvedType, int flags,
                                    List<BPackage.Service> services, int userId) {
        synchronized (mLock) {
            return mServices.queryIntentForPackage(intent, resolvedType, flags, services, userId);
        }
    }


    private static final class ServiceIntentResolver extends IntentResolver<BPackage.ServiceIntentInfo, ResolveInfo> {

        @Override
        public List<ResolveInfo> queryIntent(Intent intent, String resolvedType,
                                             boolean defaultOnly, int userId) {
            mFlags = defaultOnly ? PackageManager.MATCH_DEFAULT_ONLY : 0;
            return super.queryIntent(intent, resolvedType, defaultOnly, userId);
        }

        List<ResolveInfo> queryIntent(Intent intent, String resolvedType, int flags,
                                      int userId) {
            mFlags = flags;
            return super.queryIntent(intent, resolvedType,
                    (flags & PackageManager.MATCH_DEFAULT_ONLY) != 0,
                    userId);
        }

        List<ResolveInfo> queryIntentForPackage(Intent intent, String resolvedType,
                                                int flags, List<BPackage.Service> packageServices, int userId) {
            if (packageServices == null) {
                return null;
            }
            mFlags = flags;
            final boolean defaultOnly = (flags & PackageManager.MATCH_DEFAULT_ONLY) != 0;
            final int servicesSize = packageServices.size();
            ArrayList<BPackage.ServiceIntentInfo[]> listCut = new ArrayList<>(servicesSize);

            ArrayList<BPackage.ServiceIntentInfo> intentFilters;
            for (int i = 0; i < servicesSize; ++i) {
                intentFilters = packageServices.get(i).intents;
                if (intentFilters != null && intentFilters.size() > 0) {
                    BPackage.ServiceIntentInfo[] array =
                            new BPackage.ServiceIntentInfo[intentFilters.size()];
                    intentFilters.toArray(array);
                    listCut.add(array);
                }
            }
            return super.queryIntentFromList(intent, resolvedType, defaultOnly, listCut, userId);
        }

        void addService(BPackage.Service s) {
            mServices.put(s.getComponentName(), s);
            final int intentsSize = s.intents.size();
            int j;
            for (j = 0; j < intentsSize; j++) {
                BPackage.ServiceIntentInfo intent = s.intents.get(j);
                addFilter(intent);
            }
        }

        void removeService(BPackage.Service s) {
            mServices.remove(s.getComponentName());
            final int intentsSize = s.intents.size();
            int j;
            for (j = 0; j < intentsSize; j++) {
                BPackage.ServiceIntentInfo intent = s.intents.get(j);
                removeFilter(intent);
            }
        }

        @Override
        protected boolean isPackageForFilter(String packageName,
                                             BPackage.ServiceIntentInfo info) {
            return packageName.equals(info.service.owner.packageName);
        }

        @Override
        protected BPackage.ServiceIntentInfo[] newArray(int size) {
            return new BPackage.ServiceIntentInfo[size];
        }

        @Override
        protected ResolveInfo newResult(BPackage.ServiceIntentInfo filter, int match, int userId) {
            final BPackage.ServiceIntentInfo info = (BPackage.ServiceIntentInfo) filter;
            final BPackage.Service service = info.service;
            BPackageSettings ps = (BPackageSettings) service.owner.mExtras;
            if (ps == null) {
                return null;
            }
            ServiceInfo si = PackageManagerCompat.generateServiceInfo(service, mFlags, ps.readUserState(userId), userId);

            final ResolveInfo res = new ResolveInfo();
            res.serviceInfo = si;
            if ((mFlags & PackageManager.GET_RESOLVED_FILTER) != 0) {
                res.filter = filter.intentFilter;
            }
            res.priority = info.intentFilter.getPriority();
            res.preferredOrder = service.owner.mPreferredOrder;
            res.match = match;
            res.isDefault = info.hasDefault;
            res.labelRes = info.labelRes;
            res.nonLocalizedLabel = info.nonLocalizedLabel;
            res.icon = info.icon;
            return res;
        }

        // Keys are String (activity class name), values are Activity.
        private final ArrayMap<ComponentName, BPackage.Service> mServices = new ArrayMap<>();
        private int mFlags;
    }


    private static final class ActivityIntentResolver extends IntentResolver<BPackage.ActivityIntentInfo, ResolveInfo> {

        @Override
        public List<ResolveInfo> queryIntent(Intent intent, String resolvedType,
                                             boolean defaultOnly, int userId) {
            mFlags = (defaultOnly ? PackageManager.MATCH_DEFAULT_ONLY : 0);
            return super.queryIntent(intent, resolvedType, defaultOnly, userId);
        }

        List<ResolveInfo> queryIntent(Intent intent, String resolvedType, int flags,
                                      int userId) {
            mFlags = flags;
            return super.queryIntent(intent, resolvedType,
                    (flags & PackageManager.MATCH_DEFAULT_ONLY) != 0,
                    userId);
        }

        List<ResolveInfo> queryIntentForPackage(Intent intent, String resolvedType,
                                                int flags, List<BPackage.Activity> packageActivities, int userId) {
            if (packageActivities == null) {
                return null;
            }
            mFlags = flags;
            final boolean defaultOnly = (flags & PackageManager.MATCH_DEFAULT_ONLY) != 0;
            final int activitiesSize = packageActivities.size();
            ArrayList<BPackage.ActivityIntentInfo[]> listCut = new ArrayList<>(activitiesSize);

            ArrayList<BPackage.ActivityIntentInfo> intentFilters;
            for (int i = 0; i < activitiesSize; ++i) {
                intentFilters = packageActivities.get(i).intents;
                if (intentFilters != null && intentFilters.size() > 0) {
                    BPackage.ActivityIntentInfo[] array =
                            new BPackage.ActivityIntentInfo[intentFilters.size()];
                    intentFilters.toArray(array);
                    listCut.add(array);
                }
            }
            return super.queryIntentFromList(intent, resolvedType, defaultOnly, listCut, userId);
        }

        private void addActivity(BPackage.Activity a, String type,
                                 List<BPackage.ActivityIntentInfo> newIntents) {
            mActivities.put(a.getComponentName(), a);
            final int intentsSize = a.intents.size();
            for (int j = 0; j < intentsSize; j++) {
                BPackage.ActivityIntentInfo intent = a.intents.get(j);
                if (newIntents != null && "activity".equals(type)) {
                    newIntents.add(intent);
                    addFilter(intent);
                }
            }
        }

        private void removeActivity(BPackage.Activity a, String type) {
            mActivities.remove(a.getComponentName());
            final int intentsSize = a.intents.size();
            for (int j = 0; j < intentsSize; j++) {
                BPackage.ActivityIntentInfo intent = a.intents.get(j);
                removeFilter(intent);
            }
        }

        @Override
        protected boolean isPackageForFilter(String packageName,
                                             BPackage.ActivityIntentInfo info) {
            return packageName.equals(info.activity.owner.packageName);
        }

        @Override
        protected BPackage.ActivityIntentInfo[] newArray(int size) {
            return new BPackage.ActivityIntentInfo[size];
        }

        @Override
        protected ResolveInfo newResult(BPackage.ActivityIntentInfo info, int match, int userId) {
            final BPackage.Activity activity = info.activity;
            BPackageSettings ps = (BPackageSettings) activity.owner.mExtras;
            if (ps == null) {
                return null;
            }
            ActivityInfo ai =
                    PackageManagerCompat.generateActivityInfo(activity, mFlags, ps.readUserState(userId), userId);

            final ResolveInfo res = new ResolveInfo();
            res.activityInfo = ai;
            if ((mFlags & PackageManager.GET_RESOLVED_FILTER) != 0) {
                res.filter = info.intentFilter;
            }
            res.priority = info.intentFilter.getPriority();
            res.preferredOrder = activity.owner.mPreferredOrder;
            //System.out.println("Result: " + res.activityInfo.className +
            //                   " = " + res.priority);
            res.match = match;
            res.isDefault = info.hasDefault;
            res.labelRes = info.labelRes;
            res.nonLocalizedLabel = info.nonLocalizedLabel;
            res.icon = info.icon;
            return res;
        }

        // Keys are String (activity class name), values are Activity.
        private final ArrayMap<ComponentName, BPackage.Activity> mActivities =
                new ArrayMap<>();
        private int mFlags;
    }

    private static final class ProviderIntentResolver
            extends IntentResolver<BPackage.ProviderIntentInfo, ResolveInfo> {
        @Override
        public List<ResolveInfo> queryIntent(Intent intent, String resolvedType,
                                             boolean defaultOnly, int userId) {
            mFlags = defaultOnly ? PackageManager.MATCH_DEFAULT_ONLY : 0;
            return super.queryIntent(intent, resolvedType, defaultOnly, userId);
        }

        List<ResolveInfo> queryIntent(Intent intent, String resolvedType, int flags,
                                      int userId) {
            mFlags = flags;
            return super.queryIntent(intent, resolvedType,
                    (flags & PackageManager.MATCH_DEFAULT_ONLY) != 0,
                    userId);
        }

        List<ResolveInfo> queryIntentForPackage(Intent intent, String resolvedType,
                                                int flags, List<BPackage.Provider> packageProviders, int userId) {
            if (packageProviders == null) {
                return null;
            }
            mFlags = flags;
            final boolean defaultOnly = (flags & PackageManager.MATCH_DEFAULT_ONLY) != 0;
            final int providersSize = packageProviders.size();
            ArrayList<BPackage.ProviderIntentInfo[]> listCut = new ArrayList<>(providersSize);

            ArrayList<BPackage.ProviderIntentInfo> intentFilters;
            for (int i = 0; i < providersSize; ++i) {
                intentFilters = packageProviders.get(i).intents;
                if (intentFilters != null && intentFilters.size() > 0) {
                    BPackage.ProviderIntentInfo[] array =
                            new BPackage.ProviderIntentInfo[intentFilters.size()];
                    intentFilters.toArray(array);
                    listCut.add(array);
                }
            }
            return super.queryIntentFromList(intent, resolvedType, defaultOnly, listCut, userId);
        }

        void addProvider(BPackage.Provider p) {
            mProviders.put(p.getComponentName(), p);
            final int intentsSize = p.intents.size();
            int j;
            for (j = 0; j < intentsSize; j++) {
                BPackage.ProviderIntentInfo intent = p.intents.get(j);
                addFilter(intent);
            }
        }

        void removeProvider(BPackage.Provider p) {
            mProviders.remove(p.getComponentName());
            final int intentsSize = p.intents.size();
            int j;
            for (j = 0; j < intentsSize; j++) {
                BPackage.ProviderIntentInfo intent = p.intents.get(j);
                removeFilter(intent);
            }
        }

        @Override
        protected boolean allowFilterResult(
                BPackage.ProviderIntentInfo filter, List<ResolveInfo> dest) {
            ProviderInfo filterPi = filter.provider.info;
            for (int i = dest.size() - 1; i >= 0; i--) {
                ProviderInfo destPi = dest.get(i).providerInfo;
                if (destPi.name.equals(filterPi.name)
                        && destPi.packageName.equals(filterPi.packageName)) {
                    return false;
                }
            }
            return true;
        }

        @Override
        protected BPackage.ProviderIntentInfo[] newArray(int size) {
            return new BPackage.ProviderIntentInfo[size];
        }

        @Override
        protected boolean isPackageForFilter(String packageName,
                                             BPackage.ProviderIntentInfo info) {
            return packageName.equals(info.provider.owner.packageName);
        }

        @Override
        protected ResolveInfo newResult(BPackage.ProviderIntentInfo filter, int match, int userId) {
            final BPackage.ProviderIntentInfo info = filter;
            final BPackage.Provider provider = info.provider;
            BPackageSettings ps = (BPackageSettings) provider.owner.mExtras;
            if (ps == null) {
                return null;
            }

            ProviderInfo pi = PackageManagerCompat.generateProviderInfo(provider, mFlags, ps.readUserState(userId), userId);
            final ResolveInfo res = new ResolveInfo();
            res.providerInfo = pi;
            if ((mFlags & PackageManager.GET_RESOLVED_FILTER) != 0) {
                res.filter = filter.intentFilter;
            }
            res.priority = info.intentFilter.getPriority();
            res.preferredOrder = provider.owner.mPreferredOrder;
            res.match = match;
            res.isDefault = info.hasDefault;
            res.labelRes = info.labelRes;
            res.nonLocalizedLabel = info.nonLocalizedLabel;
            res.icon = info.icon;
            return res;
        }

        private final ArrayMap<ComponentName, BPackage.Provider> mProviders = new ArrayMap<>();
        private int mFlags;
    }
}
