package top.niunaijun.blackbox.utils;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.security.DigestException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.zip.Adler32;

/**
 * Created by Milk on 2021/6/6.
 * * ∧＿∧
 * (`･ω･∥
 * 丶　つ０
 * しーＪ
 * 此处无Bug
 */
public class DexUtils {

    public static void fixDex(File dex) {
        if (dex == null)
            return;
        FileInputStream in = null;
        FileOutputStream out = null;
        File dexN = new File(dex.getAbsolutePath() + "_fix.dex");
        try {
            in = new FileInputStream(dex);
            out = new FileOutputStream(dexN);
            byte[] bytes = new byte[in.available()];
            int read = in.read(bytes);
            if (read > 0) {
                fixFileSizeHeader(bytes);
                calcSignature(bytes);
                calcChecksum(bytes);
            }
            out.write(bytes);
            out.flush();
        } catch (Throwable e) {
            e.printStackTrace();
        } finally {
            CloseUtils.close(in, out);
            FileUtils.deleteDir(dex);
            FileUtils.renameTo(dexN, dex);
            FileUtils.deleteDir(dexN);
        }
    }

    private static void calcChecksum(byte[] var0) {
        Adler32 var2 = new Adler32();
        var2.update(var0, 12, var0.length - 12);
        int var1 = (int) var2.getValue();
        var0[8] = (byte) var1;
        var0[9] = (byte) (var1 >> 8);
        var0[10] = (byte) (var1 >> 16);
        var0[11] = (byte) (var1 >> 24);
    }

    private static void calcSignature(byte[] bytes) {
        MessageDigest md;
        try {
            md = MessageDigest.getInstance("SHA-1");
        } catch (NoSuchAlgorithmException ex) {
            throw new RuntimeException(ex);
        }

        md.update(bytes, 32, bytes.length - 32);

        try {
            int amt = md.digest(bytes, 12, 20);
            if (amt != 20) {
                throw new RuntimeException("unexpected digest write: " + amt +
                        " bytes");
            }
        } catch (DigestException ex) {
            throw new RuntimeException(ex);
        }
    }

    private static void fixFileSizeHeader(byte[] dexBytes) {
        byte[] newfs = intToByte(dexBytes.length);
        byte[] refs = new byte[4];
        for (int i = 0; i < 4; i++) {
            refs[i] = newfs[newfs.length - 1 - i];
        }
        System.arraycopy(refs, 0, dexBytes, 32, 4);
    }

    private static byte[] intToByte(int number) {
        byte[] b = new byte[4];
        for (int i = 3; i >= 0; i--) {
            b[i] = (byte) (number % 256);
            number >>= 8;
        }
        return b;
    }
}
