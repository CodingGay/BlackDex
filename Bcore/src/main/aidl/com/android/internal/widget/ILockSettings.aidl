package com.android.internal.widget;

interface ILockSettings {
    void setRecoverySecretTypes(in int[] secretTypes);
    int[] getRecoverySecretTypes();
}