package mirror;

import java.lang.reflect.AccessibleObject;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Member;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

/**
 * Created by canyie on 2019/10/24.
 */
@SuppressWarnings({"unchecked", "unused", "WeakerAccess"})
public final class MirrorReflection {
    
    private Class<?> clazz;

    private MirrorReflection( Class<?> clazz) {
        this.clazz = clazz;
    }

    
    public Class<?> getClazz() {
        return clazz;
    }

    public static MirrorReflection on( String name) {
        return new MirrorReflection(findClass(name));
    }

    public static MirrorReflection on( String name,  ClassLoader loader) {
        return new MirrorReflection(findClass(name, loader));
    }

    public static <T> MirrorReflection on( Class<T> clazz) {
        return new MirrorReflection(clazz);
    }

    public static <T> MethodWrapper<T> wrap( Method method) {
        return new MethodWrapper<T>(method);
    }

    public static <T> StaticMethodWrapper<T> wrapStatic( Method method) {
        return new StaticMethodWrapper<T>(method);
    }

    public <T> MethodWrapper<T> method( String name,  Class<?>... parameterTypes) {
        return method(clazz, name, parameterTypes);
    }

    public static <T> MethodWrapper<T> method( String className,  String name,  Class<?>... parameterTypes) {
        return method(findClass(className), name, parameterTypes);
    }

    public static <T> MethodWrapper<T> method( Class<?> clazz,  String name,  Class<?>... parameterTypes) {
        Method method = getMethod(clazz, name, parameterTypes);
        if ((parameterTypes == null || parameterTypes.length == 0) && method == null) {
            method = findMethodNoChecks(clazz, name);
        }
        return wrap(method);
    }


    public <T> StaticMethodWrapper<T> staticMethod( String name,  Class<?>... parameterTypes) {
        return staticMethod(clazz, name, parameterTypes);
    }

    public static <T> StaticMethodWrapper<T> staticMethod( String className,  String name,  Class<?>... parameterTypes) {
        return staticMethod(findClass(className), name, parameterTypes);
    }

    public static <T> StaticMethodWrapper<T> staticMethod( Class<?> clazz,  String name,  Class<?>... parameterTypes) {
        Method method = getMethod(clazz, name, parameterTypes);
        if ((parameterTypes == null || parameterTypes.length == 0) && method == null) {
            method = findMethodNoChecks(clazz, name);
        }
        return wrapStatic(method);
    }

    public static <T> FieldWrapper<T> wrap( Field field) {
        return new FieldWrapper<>(field);
    }

    public <T> FieldWrapper<T> field( String name) {
        return field(clazz, name);
    }

    public static <T> FieldWrapper<T> field( String className,  String name) {
        return field(findClass(className), name);
    }

    public static <T> FieldWrapper<T> field( Class<?> clazz,  String name) {
        return wrap(getField(clazz, name));
    }

    public static <T> ConstructorWrapper<T> wrap( Constructor<T> constructor) {
        return new ConstructorWrapper<>(constructor);
    }

    public <T> ConstructorWrapper<T> constructor( Class<?>... parameterTypes) {
        return wrap(getConstructor(clazz, parameterTypes));
    }


    
    public static Class<?> findClassOrNull( String name) {
        try {
            return Class.forName(name);
        } catch (ClassNotFoundException ignored) {
            return null;
        }
    }

    
    public static Class<?> findClassOrNull( String name,  ClassLoader loader) {
        try {
            return Class.forName(name, true, loader);
        } catch (ClassNotFoundException ignored) {
            return null;
        }
    }

    
    public static Class<?> findClass( String name) {
        try {
            return Class.forName(name);
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }
        return null;
    }

    
    public static Class<?> findClass( String name,  ClassLoader loader) {
        try {
            return Class.forName(name, true, loader);
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }
        return null;
    }

    public static Method getMethod( Class<?> clazz,  String name,  Class<?>... parameterTypes) {
        return findMethod(clazz, name, parameterTypes);
    }

    public static Method getMethod( Class<?> clazz,  String name) {
        return findMethod(clazz, name);
    }

    private static String getParameterTypesMessage( Class<?>[] parameterTypes) {
        if (parameterTypes == null || parameterTypes.length == 0) {
            return "()";
        }
        StringBuilder sb = new StringBuilder("(");
        boolean isFirst = true;
        for (Class<?> type : parameterTypes) {
            if (isFirst) {
                isFirst = false;
            } else {
                sb.append(",");
            }
            sb.append(type.getName());
        }
        return sb.append(')').toString();
    }

    
    public static Method findMethod( Class<?> clazz,  String name,  Class<?>... parameterTypes) {
        checkForFindMethod(clazz, name, parameterTypes);
        return findMethodNoChecks(clazz, name, parameterTypes);
    }

    
    public static Method findMethodNoChecks(Class<?> clazz, String name, Class<?>... parameterTypes) {
        while (clazz != null) {
            try {
                Method method = clazz.getDeclaredMethod(name, parameterTypes);
                method.setAccessible(true);
                return method;
            } catch (NoSuchMethodException ignored) {
            }
            clazz = clazz.getSuperclass();
        }
        return null;
    }

    
    public static Method findMethodNoChecks(Class<?> clazz, String name) {
        try {
            Method[] methods = clazz.getDeclaredMethods();
            for (Method method : methods) {
                if (method.getName().equals(name)) {
                    method.setAccessible(true);
                    return method;
                }
            }
        } catch (Throwable ignored) {
        }
        return null;
    }

    private static void checkForFindMethod(Class<?> clazz, String name, Class<?>... parameterTypes) {
        if (parameterTypes != null) {
            for (int i = 0; i < parameterTypes.length; i++) {
                if (parameterTypes[i] == null) {
                    throw new NullPointerException("parameterTypes[" + i + "] == null");
                }
            }
        }

    }

    
    public static Field getField( Class<?> clazz,  String name) {
        return findField(clazz, name);
    }

    
    public static Field findField(Class<?> clazz, String name) {
        return findFieldNoChecks(clazz, name);
    }

    
    public static Field findFieldNoChecks( Class<?> clazz,  String name) {
        while (clazz != null) {
            try {
                Field field = clazz.getDeclaredField(name);
                field.setAccessible(true);
                return field;
            } catch (NoSuchFieldException ignored) {
            }
            clazz = clazz.getSuperclass();
        }
        return null;
    }

    public static <T> Constructor<T> getConstructor( Class<?> clazz,  Class<?>... parameterTypes) {
        return findConstructor(clazz, parameterTypes);
    }

    public static <T> Constructor<T> findConstructor( Class<?> clazz,  Class<?>... parameterTypes) {
        checkForFindConstructor(clazz, parameterTypes);
        return findConstructorNoChecks(clazz, parameterTypes);
    }

    public static <T> Constructor<T> findConstructorNoChecks( Class<?> clazz, Class<?>... parameterTypes) {
        try {
            Constructor<T> constructor = (Constructor<T>) clazz.getDeclaredConstructor(parameterTypes);
            constructor.setAccessible(true);
            return constructor;
        } catch (NoSuchMethodException ignored) {
        }
        return null;
    }

    private static void checkForFindConstructor(Class<?> clazz, Class<?>... parameterTypes) {
        if (parameterTypes != null) {
            for (int i = 0; i < parameterTypes.length; i++) {
                if (parameterTypes[i] == null) {
                    throw new NullPointerException("parameterTypes[" + i + "] == null");
                }
            }
        }
    }

    public boolean isInstance( Object instance) {
        return clazz.isInstance(instance);
    }

    public int getModifiers() {
        return clazz.getModifiers();
    }

    public boolean isLambdaClass() {
        return isLambdaClass(clazz);
    }


    public static boolean isLambdaClass(Class<?> clazz) {
        return clazz.getName().contains("$$Lambda$");
    }

    public static boolean isProxyClass(Class<?> clazz) {
        return Proxy.isProxyClass(clazz);
    }

    public static <T extends Throwable> void throwUnchecked(Throwable e) throws T {
        throw (T) e;
    }

    public static class MemberWrapper<M extends AccessibleObject & Member> {
        M member;

        MemberWrapper(M member) {
            if (member == null)
                return;
            member.setAccessible(true);
            this.member = member;
        }

        
        public final M unwrap() {
            return member;
        }

        public final int getModifiers() {
            return member.getModifiers();
        }

        public final Class<?> getDeclaringClass() {
            return member.getDeclaringClass();
        }
    }

    public static class MethodWrapper<T> extends MemberWrapper<Method> {
        MethodWrapper(Method method) {
            super(method);
        }

        public T call(Object instance, Object... args) {
            try {
                return (T) member.invoke(instance, args);
            } catch (Throwable ignored) {
            }
            return null;
        }

        public T callWithException(Object instance, Object... args) throws Throwable {
            return (T) member.invoke(instance, args);
        }
    }

    public static class StaticMethodWrapper<T> extends MemberWrapper<Method> {
        StaticMethodWrapper(Method method) {
            super(method);
        }

        public T call(Object... args) {
            try {
                return (T) member.invoke(null, args);
            } catch (Throwable ignored) {
            }
            return null;
        }

        public T callWithException(Object... args) throws Throwable {
            return (T) member.invoke(null, args);
        }
    }

    public static class FieldWrapper<T> extends MemberWrapper<Field> {
        FieldWrapper(Field field) {
            super(field);
        }

        public T get(Object instance) {
            try {
                return (T) member.get(instance);
            } catch (Throwable ignored) {
                return null;
            }
        }

        public T get() {
            return get(null);
        }

        public void set(Object instance, Object value) {
            try {
                member.set(instance, value);
            } catch (Throwable ignored) {
            }
        }

        public void set(Object value) {
            set(null, value);
        }

        public Class<?> getType() {
            return member.getType();
        }
    }

    public static class ConstructorWrapper<T> extends MemberWrapper<Constructor<T>> {
        ConstructorWrapper(Constructor<T> constructor) {
            super(constructor);
        }

        public T newInstance(Object... args) {
            try {
                return member.newInstance(args);
            } catch (Throwable ignored) {
                return null;
            }
        }
    }
}