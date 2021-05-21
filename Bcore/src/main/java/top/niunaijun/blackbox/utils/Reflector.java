package top.niunaijun.blackbox.utils;



import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Member;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

/**
 * Created by qiaopu on 2018/4/26.
 */
public class Reflector {
    public static final String LOG_TAG ="Reflector";
    
    protected Class<?> mType;
    protected Object mCaller;
    protected Constructor mConstructor;
    protected Field mField;
    protected Method mMethod;
    
    
    public static Reflector on(String name) throws Exception {
        return on(name, true, Reflector.class.getClassLoader());
    }
    
    public static Reflector on(String name, boolean initialize) throws Exception {
        return on(name, initialize, Reflector.class.getClassLoader());
    }
    
    public static Reflector on(String name, boolean initialize, ClassLoader loader) throws Exception {
        try {
            return on(Class.forName(name, initialize, loader));
        } catch (Throwable e) {
            throw new Exception("Oops!", e);
        }
    }
    
    public static Reflector on(Class<?> type) {
        Reflector reflector = new Reflector();
        reflector.mType = type;
        return reflector;
    }
    
    public static Reflector with(Object caller) throws Exception {
        return on(caller.getClass()).bind(caller);
    }
    
    protected Reflector() {
    
    }
    
    public Reflector constructor(Class<?>... parameterTypes) throws Exception {
        try {
            mConstructor = mType.getDeclaredConstructor(parameterTypes);
            mConstructor.setAccessible(true);
            mField = null;
            mMethod = null;
            return this;
        } catch (Throwable e) {
            throw new Exception("Oops!", e);
        }
    }
    
    @SuppressWarnings("unchecked")
    public <R> R newInstance(Object... initargs) throws Exception {
        if (mConstructor == null) {
            throw new Exception("Constructor was null!");
        }
        try {
            return (R) mConstructor.newInstance(initargs);
        } catch (InvocationTargetException e) {
            throw new Exception("Oops!", e.getTargetException());
        } catch (Throwable e) {
            throw new Exception("Oops!", e);
        }
    }
    
    protected Object checked(Object caller) throws Exception {
        if (caller == null || mType.isInstance(caller)) {
            return caller;
        }
        throw new Exception("Caller [" + caller + "] is not a instance of type [" + mType + "]!");
    }
    
    protected void check(Object caller, Member member, String name) throws Exception {
        if (member == null) {
            throw new Exception(name + " was null!");
        }
        if (caller == null && !Modifier.isStatic(member.getModifiers())) {
            throw new Exception("Need a caller!");
        }
        checked(caller);
    }
    
    public Reflector bind(Object caller) throws Exception {
        mCaller = checked(caller);
        return this;
    }
    
    public Reflector unbind() {
        mCaller = null;
        return this;
    }
    
    public Reflector field(String name) throws Exception {
        try {
            mField = findField(name);
            mField.setAccessible(true);
            mConstructor = null;
            mMethod = null;
            return this;
        } catch (Throwable e) {
            throw new Exception("Oops!", e);
        }
    }
    
    protected Field findField(String name) throws NoSuchFieldException {
        try {
            return mType.getField(name);
        } catch (NoSuchFieldException e) {
            for (Class<?> cls = mType; cls != null; cls = cls.getSuperclass()) {
                try {
                    return cls.getDeclaredField(name);
                } catch (NoSuchFieldException ex) {
                    // Ignored
                }
            }
            throw e;
        }
    }
    
    @SuppressWarnings("unchecked")
    public <R> R get() throws Exception {
        return get(mCaller);
    }
    
    @SuppressWarnings("unchecked")
    public <R> R get(Object caller) throws Exception {
        check(caller, mField, "Field");
        try {
            return (R) mField.get(caller);
        } catch (Throwable e) {
            throw new Exception("Oops!", e);
        }
    }
    
    public Reflector set(Object value) throws Exception {
        return set(mCaller, value);
    }
    
    public Reflector set(Object caller, Object value) throws Exception {
        check(caller, mField, "Field");
        try {
            mField.set(caller, value);
            return this;
        } catch (Throwable e) {
            throw new Exception("Oops!", e);
        }
    }
    
    public Reflector method(String name, Class<?>... parameterTypes) throws Exception {
        try {
            mMethod = findMethod(name, parameterTypes);
            mMethod.setAccessible(true);
            mConstructor = null;
            mField = null;
            return this;
        } catch (NoSuchMethodException e) {
            throw new Exception("Oops!", e);
        }
    }
    
    protected Method findMethod(String name, Class<?>... parameterTypes) throws NoSuchMethodException {
        try {
            return mType.getMethod(name, parameterTypes);
        } catch (NoSuchMethodException e) {
            for (Class<?> cls = mType; cls != null; cls = cls.getSuperclass()) {
                try {
                    return cls.getDeclaredMethod(name, parameterTypes);
                } catch (NoSuchMethodException ex) {
                    // Ignored
                }
            }
            throw e;
        }
    }
    
    public <R> R call(Object... args) throws Exception {
        return callByCaller(mCaller, args);
    }
    
    @SuppressWarnings("unchecked")
    public <R> R callByCaller(Object caller, Object... args) throws Exception {
        check(caller, mMethod, "Method");
        try {
            return (R) mMethod.invoke(caller, args);
        } catch (InvocationTargetException e) {
            throw new Exception("Oops!", e.getTargetException());
        } catch (Throwable e) {
            throw new Exception("Oops!", e);
        }
    }
    
    public static class QuietReflector extends Reflector {
        
        protected Throwable mIgnored;
    
        public static QuietReflector on(String name) {
            return on(name, true, QuietReflector.class.getClassLoader());
        }
    
        public static QuietReflector on(String name, boolean initialize) {
            return on(name, initialize, QuietReflector.class.getClassLoader());
        }
    
        public static QuietReflector on(String name, boolean initialize, ClassLoader loader) {
            Class<?> cls = null;
            try {
                cls = Class.forName(name, initialize, loader);
                return on(cls, null);
            } catch (Throwable e) {
//                Log.w(LOG_TAG, "Oops!", e);
                return on(cls, e);
            }
        }
    
        public static QuietReflector on(Class<?> type) {
            return on(type, (type == null) ? new Exception("Type was null!") : null);
        }
    
        private static QuietReflector on(Class<?> type, Throwable ignored) {
            QuietReflector reflector = new QuietReflector();
            reflector.mType = type;
            reflector.mIgnored = ignored;
            return reflector;
        }
    
        public static QuietReflector with(Object caller) {
            if (caller == null) {
                return on((Class<?>) null);
            }
            return on(caller.getClass()).bind(caller);
        }
        
        protected QuietReflector() {
            
        }
    
        public Throwable getIgnored() {
            return mIgnored;
        }
    
        protected boolean skip() {
            return skipAlways() || mIgnored != null;
        }
        
        protected boolean skipAlways() {
            return mType == null;
        }
    
        @Override
        public QuietReflector constructor(Class<?>... parameterTypes) {
            if (skipAlways()) {
                return this;
            }
            try {
                mIgnored = null;
                super.constructor(parameterTypes);
            } catch (Throwable e) {
                mIgnored = e;
//                Log.w(LOG_TAG, "Oops!", e);
            }
            return this;
        }
    
        @Override
        public <R> R newInstance(Object... initargs) {
            if (skip()) {
                return null;
            }
            try {
                mIgnored = null;
                return super.newInstance(initargs);
            } catch (Throwable e) {
                mIgnored = e;
//                Log.w(LOG_TAG, "Oops!", e);
            }
            return null;
        }
    
        @Override
        public QuietReflector bind(Object obj) {
            if (skipAlways()) {
                return this;
            }
            try {
                mIgnored = null;
                super.bind(obj);
            } catch (Throwable e) {
                mIgnored = e;
//                Log.w(LOG_TAG, "Oops!", e);
            }
            return this;
        }
    
        @Override
        public QuietReflector unbind() {
            super.unbind();
            return this;
        }
    
        @Override
        public QuietReflector field(String name) {
            if (skipAlways()) {
                return this;
            }
            try {
                mIgnored = null;
                super.field(name);
            } catch (Throwable e) {
                mIgnored = e;
//                Log.w(LOG_TAG, "Oops!", e);
            }
            return this;
        }
    
        @Override
        public <R> R get() {
            if (skip()) {
                return null;
            }
            try {
                mIgnored = null;
                return super.get();
            } catch (Throwable e) {
                mIgnored = e;
//                Log.w(LOG_TAG, "Oops!", e);
            }
            return null;
        }
    
        @Override
        public <R> R get(Object caller) {
            if (skip()) {
                return null;
            }
            try {
                mIgnored = null;
                return super.get(caller);
            } catch (Throwable e) {
                mIgnored = e;
//                Log.w(LOG_TAG, "Oops!", e);
            }
            return null;
        }
    
        @Override
        public QuietReflector set(Object value) {
            if (skip()) {
                return this;
            }
            try {
                mIgnored = null;
                super.set(value);
            } catch (Throwable e) {
                mIgnored = e;
//                Log.w(LOG_TAG, "Oops!", e);
            }
            return this;
        }
    
        @Override
        public QuietReflector set(Object caller, Object value) {
            if (skip()) {
                return this;
            }
            try {
                mIgnored = null;
                super.set(caller, value);
            } catch (Throwable e) {
                mIgnored = e;
//                Log.w(LOG_TAG, "Oops!", e);
            }
            return this;
        }
    
        @Override
        public QuietReflector method(String name, Class<?>... parameterTypes) {
            if (skipAlways()) {
                return this;
            }
            try {
                mIgnored = null;
                super.method(name, parameterTypes);
            } catch (Throwable e) {
                mIgnored = e;
//                Log.w(LOG_TAG, "Oops!", e);
            }
            return this;
        }
    
        @Override
        public <R> R call(Object... args)  {
            if (skip()) {
                return null;
            }
            try {
                mIgnored = null;
                return super.call(args);
            } catch (Throwable e) {
                mIgnored = e;
//                Log.w(LOG_TAG, "Oops!", e);
            }
            return null;
        }
    
        @Override
        public <R> R callByCaller(Object caller, Object... args) {
            if (skip()) {
                return null;
            }
            try {
                mIgnored = null;
                return super.callByCaller(caller, args);
            } catch (Throwable e) {
                mIgnored = e;
//                Log.w(LOG_TAG, "Oops!", e);
            }
            return null;
        }
    }
}
