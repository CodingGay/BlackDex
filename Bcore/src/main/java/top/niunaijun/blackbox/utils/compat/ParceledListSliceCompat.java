package top.niunaijun.blackbox.utils.compat;

import java.lang.reflect.Method;
import java.util.List;

import reflection.android.content.pm.ParceledListSlice;
import reflection.android.content.pm.ParceledListSliceJBMR2;

public class ParceledListSliceCompat {

	public static boolean isReturnParceledListSlice(Method method) {
		return method != null && method.getReturnType() == ParceledListSlice.REF.getClazz();
	}

	public static boolean isParceledListSlice(Object obj) {
		return obj != null && obj.getClass() == ParceledListSlice.REF.getClazz();
	}

	public static Object create(List<?> list) {
		if (ParceledListSliceJBMR2.constructor != null) {
			return ParceledListSliceJBMR2.constructor.newInstance(list);
		} else {
			Object slice = ParceledListSlice.constructor.newInstance();
			for (Object item : list) {
				ParceledListSlice.append.call(slice, item);
			}
			ParceledListSlice.setLastSlice.call(slice, true);
			return slice;
		}
	}

}
