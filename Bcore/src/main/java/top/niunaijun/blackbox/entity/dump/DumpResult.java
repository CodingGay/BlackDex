package top.niunaijun.blackbox.entity.dump;

import android.os.Parcel;
import android.os.Parcelable;

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
    private static final int STATUS_RUNNING = 0;
    private static final int STATUS_SUCCESS = 1;
    private static final int STATUS_FAIL = 2;

    public String packageName;
    public String msg;
    public String dir;

    private int status = STATUS_RUNNING;
    public int totalProcess;
    public int currProcess;

    public DumpResult dumpError(String msg) {
        this.msg = msg;
        this.status = STATUS_FAIL;
        return this;
    }

    public DumpResult dumpProcess(int totalProcess, int currProcess) {
        this.totalProcess = totalProcess;
        this.currProcess = currProcess;
        this.status = STATUS_RUNNING;
        return this;
    }

    public DumpResult dumpSuccess() {
        this.status = STATUS_SUCCESS;
        return this;
    }

    public boolean isSuccess() {
        return status == STATUS_SUCCESS;
    }

    public boolean isFail() {
        return status == STATUS_FAIL;
    }

    public boolean isRunning() {
        return status == STATUS_RUNNING;
    }

    @Override
    public String toString() {
        return "DumpResult{" +
                "packageName='" + packageName + '\'' +
                ", msg='" + msg + '\'' +
                ", dir='" + dir + '\'' +
                ", status=" + status +
                ", totalProcess=" + totalProcess +
                ", currProcess=" + currProcess +
                '}';
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(this.packageName);
        dest.writeString(this.msg);
        dest.writeString(this.dir);
        dest.writeInt(this.status);
        dest.writeInt(this.totalProcess);
        dest.writeInt(this.currProcess);
    }

    public void readFromParcel(Parcel source) {
        this.packageName = source.readString();
        this.msg = source.readString();
        this.dir = source.readString();
        this.status = source.readInt();
        this.totalProcess = source.readInt();
        this.currProcess = source.readInt();
    }

    public DumpResult() {
    }

    protected DumpResult(Parcel in) {
        this.packageName = in.readString();
        this.msg = in.readString();
        this.dir = in.readString();
        this.status = in.readInt();
        this.totalProcess = in.readInt();
        this.currProcess = in.readInt();
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
