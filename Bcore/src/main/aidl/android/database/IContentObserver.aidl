package android.database;

import android.net.Uri;

interface IContentObserver
{
    /**
     * This method is called when an update occurs to the cursor that is being
     * observed. selfUpdate is true if the update was caused by a call to
     * commit on the cursor that is being observed.
     */
    void onChange(boolean selfUpdate, in Uri uri, int userId);
}
