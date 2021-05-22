package top.niunaijun.blackbox.entity.dump;

import android.os.Parcel;
import android.os.Parcelable;

import top.niunaijun.blackbox.entity.pm.InstallResult;
import top.niunaijun.blackbox.utils.Slog;

/**
 * Created by Milk on 2021/5/22.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class DumpResult implements Parcelable {
    public static final String TAG = "DumpResult";

    public boolean success = true;
    public String packageName;
    public String msg;
    public String dir;

    public void dumpError(String msg) {
        this.msg = msg;
        this.success = false;
        Slog.d(TAG, msg);
    }

    @Override
    public String toString() {
        return "DumpResult{" +
                "success=" + success +
                ", packageName='" + packageName + '\'' +
                ", msg='" + msg + '\'' +
                ", dir='" + dir + '\'' +
                '}';
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeByte(this.success ? (byte) 1 : (byte) 0);
        dest.writeString(this.packageName);
        dest.writeString(this.msg);
        dest.writeString(this.dir);
    }

    public void readFromParcel(Parcel source) {
        this.success = source.readByte() != 0;
        this.packageName = source.readString();
        this.msg = source.readString();
        this.dir = source.readString();
    }

    public DumpResult() {
    }

    protected DumpResult(Parcel in) {
        this.success = in.readByte() != 0;
        this.packageName = in.readString();
        this.msg = in.readString();
        this.dir = in.readString();
    }

    public static final Creator<DumpResult> CREATOR = new Creator<DumpResult>() {
        @Override
        public DumpResult createFromParcel(Parcel source) {
            return new DumpResult(source);
        }

        @Override
        public DumpResult[] newArray(int size) {
            return new DumpResult[size];
        }
    };
}
