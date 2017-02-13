#pragma once

#include <memory>

namespace tigl {
#ifdef HAVE_UNIQUE_PTR
	template <T>
	using unique_ptr = std::unique_ptr<T>;

	template<typename T, typename... Args>
	auto make_unique(Args&& args) -> unique_ptr<T> {
		return unique_ptr<T>(new T(std::forward<Args>(args)...));
	}
#else
#define unique_ptr std::auto_ptr

	template <typename T>
	unique_ptr<T> make_unique() {
		return unique_ptr<T>(new T());
	}

	template <typename T, typename Arg1>
	unique_ptr<T> make_unique(const Arg1& arg1) {
		return unique_ptr<T>(new T(arg1));
	}

	template <typename T, typename Arg1, typename Arg2>
	unique_ptr<T> make_unique(const Arg1& arg1, const Arg2& arg2) {
		return unique_ptr<T>(new T(arg1, arg2));
	}

	template <typename T, typename Arg1, typename Arg2, typename Arg3>
	unique_ptr<T> make_unique(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3) {
		return unique_ptr<T>(new T(arg1, arg2, arg3));
	}
#endif
}
