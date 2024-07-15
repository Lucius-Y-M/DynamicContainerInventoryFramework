#pragma once

#include "PCH.h"

#include <utility>			// std::pair
#include <string>			// std::string, std::string_view
#include <vector>
#include <type_traits>
#include <variant>			// std::variant
#include <unordered_map>
#include <unordered_set>





/* type aliases based on Rust names */

using u8 = uint8_t;
using i8 = int8_t;
using u16 = uint16_t;
using i16 = int16_t;
using u32 = uint32_t;
using i32 = int32_t;
using u64 = uint64_t;
using i64 = int64_t;
using usize = size_t;

using f32 = float;
using f64 = double;


using CString = const char *;
using String = std::string;
using Str = std::string_view;

// this is from Java rather than Rust
using StringBuilder = std::stringstream;



template<typename T>
using Uniq = std::unique_ptr<T>;



#define STAT_U8 static_cast<u8>
#define STAT_U32 static_cast<u32>

#define ASSERT static_assert



namespace Mat {
	int add(u32 a, u32 b);
};



using Once = std::once_flag;
#define InitOnce std::call_once





#define STAT        static_cast
#define STAT_8      static_cast<u8>
#define STAT_32     static_cast<u32>






template<typename T, usize N>
using Array = std::array<T, N>;

template<typename T>
using Vec = std::vector<T>;

template<typename K, typename V>
using HashMap = std::unordered_map<K, V>;

template<typename T>
using HashSet = std::unordered_set<T>;


template<typename T>
using Option = std::optional<T>;
using NONE = std::nullopt_t;
static constexpr inline auto None = std::nullopt;

template<typename T>
static inline Option<T> Some(T data) {
	return std::make_optional(data);
}


template<typename ... T>
using Fn = std::function<T...>;


template<typename Ok, typename Err>
using Result = std::variant<Ok, Err>;
template<typename K>
using Ok = Result<K, Str>;

template<typename Ok, typename Err>
static constexpr inline bool IsOk(const Result<Ok, Err> & res) {
	return std::holds_alternative<Ok>(res);
}
template<typename Ok, typename Err>
static constexpr inline bool IsErr(const Result<Ok, Err> & res) {
	return std::holds_alternative<Err>(res);
}

template<typename Ok, typename Err>
static constexpr inline Option<Ok> TryGetOk(const Result<Ok, Err> & res) {
	if (IsOk(res)) {
		return std::get<Ok>(res);
	}
	return None;
}

template<typename Ok, typename Err>
static constexpr inline Option<Ok> TryGetErr(Result<Ok, Err> res) {
	if (IsErr(res)) {
		return std::get<Err>(res);
	}
	return None;
}







template <typename A, typename B>
using Pair = std::pair<A, B>;
template <typename A, typename B>
static constexpr inline Pair<A, B> APair(A a, B b) { return std::make_pair(a,b); }


// template <typename A, typename B, typename C>
// using Tup3 = std::tuple<A, B, C>;

	// template <typename A, typename B, typename C>
	// static inline constexpr auto TupGet(const Tuple<A,B,C> & tuple, usize N) {
	// 	ASSERT(N < 3);
	// 	return std::get<N>(tuple);
	// }

	template<usize N>
	#define TupGet(N, tuple) std::get<N>(tuple)


template <typename A, typename B, typename C, typename D>
using Tup4 = std::tuple<A, B, C, D>;

template<typename ... R>
using Tuple = std::tuple<R...>;
template <typename ... Args>
static constexpr inline Tuple<Args...> ATuple(Args & ... args) { return std::make_tuple(std::forward<Args>(args)...); }








/*
	C++ enums cannot have their own data types
	so have to use this
*/
template<typename ... T>
using RustEnum = std::variant<T...>;

template<typename Wanted, typename ... T>
static constexpr inline bool Is(const RustEnum<T...> & item) {
	return std::holds_alternative<Wanted>(item);
}

template<typename Wanted, typename ... T>
static constexpr inline Option<Wanted> TryGet(const RustEnum<T...> & item) {
	if (Is<Wanted>(item)) {
		return std::get<Wanted>(item);
	}
	return None;
}

template<typename Wanted, typename ... T>
static constexpr inline Wanted UnsafeGet(const RustEnum<T...> & item) {
	return std::get<Wanted>(item);
}






// template<typename T, typename R>
// using IsSameV = std::is_same_v<T, R>;
template<typename T, typename R>
using IsSame = std::is_same<T, R>;


/*
	this is an imitation of Rust's MATCH statement for enums

	but this cannot short-circuit and early return
	due to the scope of the lambda function "capturing" any return statement
*/

template <typename... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;


template<typename Item, typename... Visitor>
static void Match(Item && value, Visitor && ... visitors) {
	std::visit(Overloaded(visitors), value);
}
template<typename Item, typename... Visitor>
static void Visit(Item && value, Visitor && ... visitors) {
	std::visit(Overloaded(visitors), value);
}



/* allow compile time check of str arrays */
template<usize N>
static constexpr bool AllOfArrayNotEmpty(Array<Str, N> array) {
	for (auto str : array) {
		if (str.empty()) { return false; }
	}
	return true;
}





/*
	A convenient function for making sure something is contained in an std::array

	(special impls for CString and Str)
*/

template<usize N>
[[maybe_unused]]
static constexpr bool ArrayContains(Array<Str, N> array, Str str) {
	for (const auto & item: array) {

		/* similar to why C Str must use "strcmp" */
		if (str.compare(item) == 0) {
			return true;
		}
	}
	return false;
}

template<usize N>
[[maybe_unused]]
static constexpr bool ArrayContains(Array<CString, N> array, CString str) {
	for (const auto & item: array) {

		/* similar to why C Str must use "strcmp" */
		if (strcmp(item, str) == 0) {
			return true;
		}
	}
	return false;
}

template<typename T, usize N>
[[maybe_unused]]
static constexpr bool ArrayContains(Array<T, N> array, T query) {
	for (const auto & item: array) {

		/* similar to why C Str must use "strcmp" */
		if (item == query) {
			return true;
		}
	}
	return false;
}






/* this is an impl of Rust's std::array::from_fn */

template<typename T, usize N>
[[maybe_unused]]
static Array<T, N> from_fn(T lambda(usize)) {
	Array<T, N> array;
	for (usize i = 0; i < N; i++) {
		array[i] = lambda(i);
	}
	return array;
}







/*
	compile-time checks for compile-time arrays
*/

template<typename T, usize N>
[[maybe_unused]]
static constexpr bool AllNotNull(const Array<T *, N> & arr) {
	for (usize i = 0; i < N; i++) {
		if (arr[i] == nullptr) {
			return false;
		}
	}

	return true;
}







/*
	imitation of Rust's "if let" syntax
*/

#define let_enum( type, myEnum, closureVal, closure )   	\
		(std::holds_alternative<type>(myEnum)) { 			\
															\
		auto closureVal = std::get<type>(myEnum);			\
		closure											    \
	}														\

#define	let_some(										\
	option, closureVal, closure							\
)	 													\
		(option.has_value()) {							\
														\
		auto closureVal = option.value();				\
														\
		closure											\
	}													\


/*
	This may seem stupid

	but it actually helps bypass the issue of "unguarded commas" in macros in C/C++
*/
#define COMMA ,

