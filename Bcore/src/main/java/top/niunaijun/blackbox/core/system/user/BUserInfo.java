package top.niunaijun.blackbox.core.system.user;

import android.os.Parcel;
import android.os.Parcelable;

import top.niunaijun.blackbox.core.system.pm.BPackage;
import top.niunaijun.blackbox.core.system.pm.BPackageSettings;

/**
 * Created by Milk on 4/22/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class BUserInfo implements Parcelable {
    public int id;
    public BUserStatus status;
    public String name;
    public long createTime;

    BUserInfo() {
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(this.id);
        dest.writeInt(this.status == null ? -1 : this.status.ordinal());
        dest.writeString(this.name);
        dest.writeLong(this.createTime);
    }

    protected BUserInfo(Parcel in) {
        this.id = in.readInt();
        int tmpStatus = in.readInt();
        this.status = tmpStatus == -1 ? null : BUserStatus.values()[tmpStatus];
        this.name = in.readString();
        this.createTime = in.readLong();
    }

    public static final Creator<BUserInfo> CREATOR = new Creator<BUserInfo>() {
        @Override
        public BUserInfo createFromParcel(Parcel source) {
            return new BUserInfo(source);
        }

        @Override
        public BUserInfo[] newArray(int size) {
            return new BUserInfo[size];
        }
    };

    @Override
    public String toString() {
        return "BUserInfo{" +
                "id=" + id +
                ", status=" + status +
                ", name='" + name + '\'' +
                ", createTime=" + createTime +
                '}';
    }
}
