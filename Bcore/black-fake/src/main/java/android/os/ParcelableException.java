package android.os;

import android.annotation.TargetApi;

import java.io.IOException;

/**
 * Wrapper class that offers to transport typical {@link Throwable} across a
 * {@link Binder} call. This class is typically used to transport exceptions
 * that cannot be modified to add {@link Parcelable} behavior, such as
 * {@link IOException}.
 * <ul>
 * <li>The wrapped throwable must be defined as system class (that is, it must
 * be in the same {@link ClassLoader} as {@link Parcelable}).
 * <li>The wrapped throwable must support the
 * {@link Throwable#Throwable(String)} constructor.
 * <li>The receiver side must catch any thrown {@link ParcelableException} and
 * call {@link #maybeRethrow(Class)} for all expected exception types.
 * </ul>
 */
@TargetApi(Build.VERSION_CODES.O)
public final class ParcelableException extends RuntimeException implements Parcelable {
    public ParcelableException(Throwable t) {
        super(t);
    }

    public <T extends Throwable> void maybeRethrow(Class<T> clazz) throws T {
        throw new RuntimeException("Stub!");
    }

    public static Throwable readFromParcel(Parcel in) {
        throw new RuntimeException("Stub!");
    }

    public static void writeToParcel(Parcel out, Throwable t) {
        throw new RuntimeException("Stub!");
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        throw new RuntimeException("Stub!");
    }

    public static final Creator<ParcelableException> CREATOR = new Creator<ParcelableException>() {
        @Override
        public ParcelableException createFromParcel(Parcel source) {
            return new ParcelableException(readFromParcel(source));
        }

        @Override
        public ParcelableException[] newArray(int size) {
            return new ParcelableException[size];
        }
    };
}
