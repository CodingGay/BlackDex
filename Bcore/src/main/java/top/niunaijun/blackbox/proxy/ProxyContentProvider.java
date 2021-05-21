package top.niunaijun.blackbox.proxy;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import top.niunaijun.blackbox.entity.AppConfig;
import top.niunaijun.blackbox.app.BActivityThread;
import top.niunaijun.blackbox.utils.compat.BundleCompat;

/**
 * Created by Milk on 3/30/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class ProxyContentProvider extends ContentProvider {
    @Override
    public boolean onCreate() {
        return false;
    }

    @Nullable
    @Override
    public Bundle call(@NonNull String method, @Nullable String arg, @Nullable Bundle extras) {
        if (method.equals("_Black_|_init_process_")) {
            assert extras != null;
            extras.setClassLoader(AppConfig.class.getClassLoader());
            AppConfig appConfig = extras.getParcelable(AppConfig.KEY);
            BActivityThread.currentActivityThread().initProcess(appConfig);

            Bundle bundle = new Bundle();
            BundleCompat.putBinder(bundle, "_Black_|_client_", BActivityThread.currentActivityThread());
            return bundle;
        }
        return super.call(method, arg, extras);
    }

    @Nullable
    @Override
    public Cursor query(@NonNull Uri uri, @Nullable String[] projection, @Nullable String selection, @Nullable String[] selectionArgs, @Nullable String sortOrder) {
        return null;
    }

    @Nullable
    @Override
    public String getType(@NonNull Uri uri) {
        return null;
    }

    @Nullable
    @Override
    public Uri insert(@NonNull Uri uri, @Nullable ContentValues values) {
        return null;
    }

    @Override
    public int delete(@NonNull Uri uri, @Nullable String selection, @Nullable String[] selectionArgs) {
        return 0;
    }

    @Override
    public int update(@NonNull Uri uri, @Nullable ContentValues values, @Nullable String selection, @Nullable String[] selectionArgs) {
        return 0;
    }

    public static class P0 extends ProxyContentProvider {

    }

    public static class P1 extends ProxyContentProvider {

    }

    public static class P2 extends ProxyContentProvider {

    }

    public static class P3 extends ProxyContentProvider {

    }

    public static class P4 extends ProxyContentProvider {

    }

    public static class P5 extends ProxyContentProvider {

    }

    public static class P6 extends ProxyContentProvider {

    }

    public static class P7 extends ProxyContentProvider {

    }

    public static class P8 extends ProxyContentProvider {

    }

    public static class P9 extends ProxyContentProvider {

    }

    public static class P10 extends ProxyContentProvider {

    }

    public static class P11 extends ProxyContentProvider {

    }

    public static class P12 extends ProxyContentProvider {

    }

    public static class P13 extends ProxyContentProvider {

    }

    public static class P14 extends ProxyContentProvider {

    }

    public static class P15 extends ProxyContentProvider {

    }

    public static class P16 extends ProxyContentProvider {

    }

    public static class P17 extends ProxyContentProvider {

    }

    public static class P18 extends ProxyContentProvider {

    }

    public static class P19 extends ProxyContentProvider {

    }

    public static class P20 extends ProxyContentProvider {

    }

    public static class P21 extends ProxyContentProvider {

    }

    public static class P22 extends ProxyContentProvider {

    }

    public static class P23 extends ProxyContentProvider {

    }

    public static class P24 extends ProxyContentProvider {

    }

    public static class P25 extends ProxyContentProvider {

    }

    public static class P26 extends ProxyContentProvider {

    }

    public static class P27 extends ProxyContentProvider {

    }

    public static class P28 extends ProxyContentProvider {

    }

    public static class P29 extends ProxyContentProvider {

    }

    public static class P30 extends ProxyContentProvider {

    }

    public static class P31 extends ProxyContentProvider {

    }

    public static class P32 extends ProxyContentProvider {

    }

    public static class P33 extends ProxyContentProvider {

    }

    public static class P34 extends ProxyContentProvider {

    }

    public static class P35 extends ProxyContentProvider {

    }

    public static class P36 extends ProxyContentProvider {

    }

    public static class P37 extends ProxyContentProvider {

    }

    public static class P38 extends ProxyContentProvider {

    }

    public static class P39 extends ProxyContentProvider {

    }

    public static class P40 extends ProxyContentProvider {

    }

    public static class P41 extends ProxyContentProvider {

    }

    public static class P42 extends ProxyContentProvider {

    }

    public static class P43 extends ProxyContentProvider {

    }

    public static class P44 extends ProxyContentProvider {

    }

    public static class P45 extends ProxyContentProvider {

    }

    public static class P46 extends ProxyContentProvider {

    }

    public static class P47 extends ProxyContentProvider {

    }

    public static class P48 extends ProxyContentProvider {

    }

    public static class P49 extends ProxyContentProvider {

    }

    public static class P50 extends ProxyContentProvider {

    }

    public static class P51 extends ProxyContentProvider {

    }

    public static class P52 extends ProxyContentProvider {

    }

    public static class P53 extends ProxyContentProvider {

    }

    public static class P54 extends ProxyContentProvider {

    }

    public static class P55 extends ProxyContentProvider {

    }

    public static class P56 extends ProxyContentProvider {

    }

    public static class P57 extends ProxyContentProvider {

    }

    public static class P58 extends ProxyContentProvider {

    }

    public static class P59 extends ProxyContentProvider {

    }

    public static class P60 extends ProxyContentProvider {

    }

    public static class P61 extends ProxyContentProvider {

    }

    public static class P62 extends ProxyContentProvider {

    }

    public static class P63 extends ProxyContentProvider {

    }

    public static class P64 extends ProxyContentProvider {

    }

    public static class P65 extends ProxyContentProvider {

    }

    public static class P66 extends ProxyContentProvider {

    }

    public static class P67 extends ProxyContentProvider {

    }

    public static class P68 extends ProxyContentProvider {

    }

    public static class P69 extends ProxyContentProvider {

    }

    public static class P70 extends ProxyContentProvider {

    }

    public static class P71 extends ProxyContentProvider {

    }

    public static class P72 extends ProxyContentProvider {

    }

    public static class P73 extends ProxyContentProvider {

    }

    public static class P74 extends ProxyContentProvider {

    }

    public static class P75 extends ProxyContentProvider {

    }

    public static class P76 extends ProxyContentProvider {

    }

    public static class P77 extends ProxyContentProvider {

    }

    public static class P78 extends ProxyContentProvider {

    }

    public static class P79 extends ProxyContentProvider {

    }

    public static class P80 extends ProxyContentProvider {

    }

    public static class P81 extends ProxyContentProvider {

    }

    public static class P82 extends ProxyContentProvider {

    }

    public static class P83 extends ProxyContentProvider {

    }

    public static class P84 extends ProxyContentProvider {

    }

    public static class P85 extends ProxyContentProvider {

    }

    public static class P86 extends ProxyContentProvider {

    }

    public static class P87 extends ProxyContentProvider {

    }

    public static class P88 extends ProxyContentProvider {

    }

    public static class P89 extends ProxyContentProvider {

    }

    public static class P90 extends ProxyContentProvider {

    }

    public static class P91 extends ProxyContentProvider {

    }

    public static class P92 extends ProxyContentProvider {

    }

    public static class P93 extends ProxyContentProvider {

    }

    public static class P94 extends ProxyContentProvider {

    }

    public static class P95 extends ProxyContentProvider {

    }

    public static class P96 extends ProxyContentProvider {

    }

    public static class P97 extends ProxyContentProvider {

    }

    public static class P98 extends ProxyContentProvider {

    }

    public static class P99 extends ProxyContentProvider {

    }
}
