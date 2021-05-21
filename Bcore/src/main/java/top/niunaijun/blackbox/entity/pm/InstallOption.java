package top.niunaijun.blackbox.entity.pm;

import android.os.Parcel;
import android.os.Parcelable;

/**
 * Created by Milk on 4/21/21.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class InstallOption implements Parcelable {
    public static final int FLAG_SYSTEM = 1;
    public static final int FLAG_STORAGE = 1 << 1;
    public static final int FLAG_URI_FILE = 1 << 3;

    public int flags = 0;

    public static InstallOption installBySystem() {
        InstallOption installOption = new InstallOption();
        installOption.flags = installOption.flags | FLAG_SYSTEM;
        return installOption;
    }

    public static InstallOption installByStorage() {
        InstallOption installOption = new InstallOption();
        installOption.flags = installOption.flags | FLAG_STORAGE;
        return installOption;
    }

    public InstallOption makeUriFile() {
        this.flags |= FLAG_URI_FILE;
        return this;
    }

    public boolean isFlag(int flag) {
        return (flags & flag) != 0;
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(this.flags);
    }

    public InstallOption() {
    }

    protected InstallOption(Parcel in) {
        this.flags = in.readInt();
    }

    public static final Parcelable.Creator<InstallOption> CREATOR = new Parcelable.Creator<InstallOption>() {
        @Override
        public InstallOption createFromParcel(Parcel source) {
            return new InstallOption(source);
        }

        @Override
        public InstallOption[] newArray(int size) {
            return new InstallOption[size];
        }
    };
}
