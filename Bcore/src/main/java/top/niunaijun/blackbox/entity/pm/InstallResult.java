package top.niunaijun.blackbox.entity.pm;

import android.os.Parcel;
import android.os.Parcelable;

import top.niunaijun.blackbox.utils.Slog;

/**
 * Created by Milk on 4/20/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class InstallResult implements Parcelable {
    public static final String TAG = "InstallResult";

    public boolean success = true;
    public String packageName;
    public String msg;

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeByte(this.success ? (byte) 1 : (byte) 0);
        dest.writeString(this.packageName);
        dest.writeString(this.msg);
    }

    public InstallResult() {
    }

    protected InstallResult(Parcel in) {
        this.success = in.readByte() != 0;
        this.packageName = in.readString();
        this.msg = in.readString();
    }

    public InstallResult installError(String msg) {
        this.msg = msg;
        this.success = false;
        Slog.d(TAG, msg);
        return this;
    }

    public static final Parcelable.Creator<InstallResult> CREATOR = new Parcelable.Creator<InstallResult>() {
        @Override
        public InstallResult createFromParcel(Parcel source) {
            return new InstallResult(source);
        }

        @Override
        public InstallResult[] newArray(int size) {
            return new InstallResult[size];
        }
    };

    @Override
    public String toString() {
        return "InstallResult{" +
                "success=" + success +
                ", packageName='" + packageName + '\'' +
                ", msg='" + msg + '\'' +
                '}';
    }
}
