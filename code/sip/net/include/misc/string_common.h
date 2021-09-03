/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef	__STRING_COMMON_H__
#define	__STRING_COMMON_H__

#include "types_sip.h"

#include <stdio.h> // <cstdio>
#include <stdarg.h> // <cstdarg>

namespace SIPBASE
{

// get a string and add \r before \n if necessary
std::string addSlashR (std::string str);
std::basic_string<ucchar> addSlashR_W (std::basic_string<ucchar> str);
std::string removeSlashR (std::string str);
std::basic_string<ucchar> removeSlashR_W (std::basic_string<ucchar> str);

/**
 * \def MaxCStringSize
 *
 * The maximum size allowed for C string (zero terminated string) buffer.
 * This value is used when we have to create a standard C string buffer and we don't know exactly the final size of the string.
 */
const int MaxCStringSize = 2048;


/**
 * \def SIPBASE_CONVERT_VARGS(dest,format)
 *
 * This macro converts variable arguments into C string (zero terminated string).
 * This function takes care to avoid buffer overflow.
 *
 * Example:
 *\code
	void MyFunction(const char *format, ...)
	{
		string str;
		SIPBASE_CONVERT_VARGS (str, format, SIPBASE::MaxCStringSize);
		// str contains the result of the conversion
	}
 *\endcode
 *
 * \param _dest \c string or \c char* that contains the result of the convertion
 * \param _format format of the string, it must be the last argument before the \c '...'
 * \param _size size of the buffer that will contain the C string
 */

#define SIPBASE_CONVERT_VARGS(_dest, _format, _size) \
char _cstring[_size]; \
va_list _args; \
va_start (_args, _format); \
int _res = _VSNPRINTFL (_cstring, _size-1, _format, _args); \
if (_res == -1 || _res == _size-1) \
{ \
	_cstring[_size-1] = '\0'; \
} \
va_end (_args); \
_dest = _cstring

#ifdef SIP_OS_WINDOWS
	#define SIPBASE_CONVERT_VARGS_WIDE(_dest, _format, _size) \
	ucchar _cstring[_size]; \
	va_list _args; \
	va_start (_args, _format); \
	int _res = _VSNWPRINTFL (_cstring, _size-1, _format, _args); \
	if (_res == -1 || _res == _size-1) \
	{ \
		_cstring[_size-1] = L'\0'; \
	} \
	va_end (_args); \
	_dest = _cstring
#else
	#define SIPBASE_CONVERT_VARGS_WIDE(_dest, _format, _size) \
	ucchar _cstring[_size]; \
	va_list _args; \
	va_start (_args, _format); \
	std::basic_string<ucchar> strFormat(_format); \
	std::basic_string<ucchar>::size_type index = 0; \
	while (true) \
	{ \
		index = strFormat.find(L"%s", index); \
		if (index == std::string::npos) \
			break; \
		strFormat.replace(index++, 2, L"%S"); \
	} \
	int _res = _VSNWPRINTFL (_cstring, _size-1, strFormat.c_str(), _args); \
	if (_res == -1 || _res == _size-1) \
	{ \
		_cstring[_size-1] = L'\0'; \
	} \
	va_end (_args); \
	_dest = _cstring
#endif

/** sMart sprintf function. This function do a sprintf and add a zero at the end of the buffer
 * if there no enough room in the buffer.
 *
 * \param buffer a C string
 * \param count Size of the buffer
 * \param format of the string, it must be the last argument before the \c '...'
 */
sint smprintf ( char   *buffer, size_t count, const char   *format, ... );
sint smprintf ( ucchar *buffer, size_t count, const ucchar *format, ... );

#ifdef SIP_OS_WINDOWS

//
// These functions are needed by template system to fail the compilation if you pass bad type on sipinfo...
//

inline void _check(int a) { }
inline void _check(unsigned int a) { }
inline void _check(char a) { }
inline void _check(unsigned char a) { }
inline void _check(long a) { }
inline void _check(unsigned long a) { }

#ifdef SIP_COMP_VC6
	inline void _check(uint8 a) { }
#endif // SIP_COMP_VC6

inline void _check(sint8 a) { }
inline void _check(uint16 a) { }
inline void _check(sint16 a) { }

#ifdef SIP_COMP_VC6
	inline void _check(uint32 a) { }
	inline void _check(sint32 a) { }
#endif // SIP_COMP_VC6

inline void _check(uint64 a) { }
inline void _check(sint64 a) { }
inline void _check(float a) { }
inline void _check(double a) { }
inline void _check(const char *a) { }
inline void _check(const ucchar *a) { }
inline void _check(const void *a) { }

#define CHECK_TYPES(__a,__b) \
template<class T> __a(const T *fmt) { __b(fmt); } \
template<class T, class A> __a(const T *fmt, A a) { _check(a); __b(fmt, a); } \
template<class T, class A, class B> __a(const T *fmt, A a, B b) { _check(a); _check(b); __b(fmt, a, b); } \
template<class T, class A, class B, class C> __a(const T *fmt, A a, B b, C c) { _check(a); _check(b); _check(c); __b(fmt, a, b, c); } \
template<class T, class A, class B, class C, class D> __a(const T *fmt, A a, B b, C c, D d) { _check(a); _check(b); _check(c); _check(d); __b(fmt, a, b, c, d); } \
template<class T, class A, class B, class C, class D, class E> __a(const T *fmt, A a, B b, C c, D d, E e) { _check(a); _check(b); _check(c); _check(d); _check(e); __b(fmt, a, b, c, d, e); } \
template<class T, class A, class B, class C, class D, class E, class F> __a(const T *fmt, A a, B b, C c, D d, E e, F f) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); __b(fmt, a, b, c, d, e, f); } \
template<class T, class A, class B, class C, class D, class E, class F, class G> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); __b(fmt, a, b, c, d, e, f, g); } \
template<class T, class A, class B, class C, class D, class E, class F, class G, class H> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g, H h) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); __b(fmt, a, b, c, d, e, f, g, h); } \
template<class T, class A, class B, class C, class D, class E, class F, class G, class H, class I> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); __b(fmt, a, b, c, d, e, f, g, h, i); } \
template<class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); __b(fmt, a, b, c, d, e, f, g, h, i, j); } \
template<class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); __b(fmt, a, b, c, d, e, f, g, h, i, j, k); } \
template<class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l); } \
template<class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m); } \
template<class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n); } \
template<class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o); } \
template<class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p); } \
template<class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q); } \
template<class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r); } \
template<class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s); } \
template<class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t); } \
template<class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT, class U> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t, U u) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); _check(u); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u); } \
template<class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT, class U, class V> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t, U u, V v) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); _check(u); _check(v); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v); } \
template<class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT, class U, class V, class W> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t, U u, V v, W w) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); _check(u); _check(v); _check(w); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w); } \
template<class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT, class U, class V, class W, class X> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t, U u, V v, W w, X x) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); _check(u); _check(v); _check(w); _check(x); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x); } \
template<class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT, class U, class V, class W, class X, class Y> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t, U u, V v, W w, X x, Y y) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); _check(u); _check(v); _check(w); _check(x); _check(y); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y); } \
template<class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT, class U, class V, class W, class X, class Y, class Z> __a(const T *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t, U u, V v, W w, X x, Y y, Z z) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); _check(u); _check(v); _check(w); _check(x); _check(y); _check(z); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z); }

#endif

#ifdef SIP_OS_WINDOWS
	inline std::basic_string<char> _toString(const char *format, ...)
#else
	inline std::basic_string<char> toString(const char *format, ...)
#endif
{
	std::basic_string<char> Result;

	SIPBASE_CONVERT_VARGS(Result, format, SIPBASE::MaxCStringSize);
	return Result;
}

#ifdef SIP_OS_WINDOWS
	inline std::basic_string<ucchar> _toString(const ucchar *format, ...)
#else
	inline std::basic_string<ucchar> toString(const ucchar *format, ...)
#endif
{
	std::basic_string<ucchar> Result;

	SIPBASE_CONVERT_VARGS_WIDE(Result, format, SIPBASE::MaxCStringSize);
	return Result;
}

//--- add by LCI 091017  --------
std::basic_string<ucchar>	getUcsFromMbs(const char * mbs);
std::string			getMbsFromUcs(const ucchar * ucs); //add by LCI 091021
std::basic_string<ucchar>	toStringLW(const char* format, ...);
//---

#ifdef SIP_OS_WINDOWS
	CHECK_TYPES(std::basic_string<T> toString, return _toString)
#endif // SIP_OS_WINDOWS

#ifdef SIP_OS_UNIX
#	define _toString toString
#endif

template<class T> std::string toStringPtr(const T *val) { return toString("%p", val); }

template<class T> std::string toStringEnum(const T &val) { return toString("%u", (uint32)val); }

/**
 * Template Object toString.
 * \param obj any object providing a "std::string toString()" method. The object doesn't have to derive from anything.
 * 
 * the VC++ error "error C2228: left of '.toString' must have class/struct/union type" means you don't provide
 * a toString() method to your object.
 */
template<class T> std::string toString(const T &obj)
{
	return obj.toString();
}

/*
//inline std::string toString(const char *val) { return toString("%s", val); }
inline tstring toString(const uint8 &val) { return toString(_t("%hu"), (uint16)val); }
inline tstring toString(const sint8 &val) { return toString(_t("%hd"), (sint16)val); }
inline tstring toString(const uint16 &val) { return toString(_t("%hu"), val); }
inline tstring toString(const sint16 &val) { return toString(_t("%hd"), val); }
inline tstring toString(const uint32 &val) { return toString(_t("%u"), val); }
inline tstring toString(const sint32 &val) { return toString(_t("%d"), val); }
inline tstring toString(const uint64 &val) { return toString(_t("%I64u"), val); }
inline tstring toString(const sint64 &val) { return toString(_t("%I64d"), val); }
inline tstring toString(const uint &val) { return toString(_t("%hu"), val); }
inline tstring toString(const sint &val) { return toString(_t("%hd"), val); }
inline tstring toString(const float &val) { return toString(_t("%f"), val); }
inline tstring toString(const double &val) { return toString(_t("%lf"), val); }
inline tstring toString(const bool &val) { return toString(_t("%u"), val?1:0); }
inline tstring toString(const tstring &val) { return val; }
*/
inline std::string toStringA(const uint8 &val) { return toString("%hu", (uint16)val); }
inline std::string toStringA(const sint8 &val) { return toString("%hd", (sint16)val); }
inline std::string toStringA(const uint16 &val) { return toString("%hu", val); }
inline std::string toStringA(const sint16 &val) { return toString("%hd", val); }
inline std::string toStringA(const uint32 &val) { return toString("%u", val); }
inline std::string toStringA(const sint32 &val) { return toString("%d", val); }
inline std::string toStringA(const uint64 &val) { return toString("%I64u", val); }
inline std::string toStringA(const sint64 &val) { return toString("%I64d", val); }
inline std::string toStringA(const float &val) { return toString("%f", val); }
inline std::string toStringA(const double &val) { return toString("%lf", val); }
inline std::string toStringA(const bool &val) { return toString("%u", val?1:0); }
inline std::string toStringA(const std::string &val) { return val; }

inline std::basic_string<ucchar> toStringW(const uint8 &val) { return toString(L"%hu", (uint16)val); }
inline std::basic_string<ucchar> toStringW(const sint8 &val) { return toString(L"%hd", (sint16)val); }
inline std::basic_string<ucchar> toStringW(const uint16 &val) { return toString(L"%hu", val); }
inline std::basic_string<ucchar> toStringW(const sint16 &val) { return toString(L"%hd", val); }
inline std::basic_string<ucchar> toStringW(const uint32 &val) { return toString(L"%u", val); }
inline std::basic_string<ucchar> toStringW(const sint32 &val) { return toString(L"%d", val); }
inline std::basic_string<ucchar> toStringW(const uint64 &val) { return toString(L"%I64u", val); }
inline std::basic_string<ucchar> toStringW(const sint64 &val) { return toString(L"%I64d", val); }
inline std::basic_string<ucchar> toStringW(const float &val) { return toString(L"%f", val); }
inline std::basic_string<ucchar> toStringW(const double &val) { return toString(L"%lf", val); }
inline std::basic_string<ucchar> toStringW(const bool &val) { return toString(L"%u", val?1:0); }
inline std::basic_string<ucchar> toStringW(const std::basic_string<ucchar> &val) { return val; }


#ifdef SIP_OS_WINDOWS
//by lci 09-05-20
// Linux������ unit->uint32, sint->sint32 �� �Ǳ⶧���� �ݺ���Ƿ\EF\BF?�ȴ�.
inline std::string toStringA(const uint &val) { return toString("%u", val); }
inline std::string toStringA(const sint &val) { return toString("%d", val); }

inline std::basic_string<ucchar> toStringW(const uint &val) { return toString(L"%u", val); }
inline std::basic_string<ucchar> toStringW(const sint &val) { return toString(L"%d", val); }
#endif
// stl vectors of bool use bit reference and not real bools, so define the operator for bit reference

// Debug : Sept 01 2006
#ifdef _STLPORT_VERSION
#ifdef SIP_OS_WINDOWS	// by ysg
#if _STLPORT_VERSION >= 0x510
	inline std::string toStringA(const std::priv::_Bit_reference &val) { return toStringA( bool(val)); }
	inline std::basic_string<ucchar> toStringW(const std::priv::_Bit_reference &val) { return toStringW( bool(val)); }

#else
	inline std::string toStringA(const std::_Bit_reference &val) { return toStringA( bool(val)); }
	inline std::basic_string<ucchar> toStringW(const std::_Bit_reference &val) { return toStringW( bool(val)); }
#endif // _STLPORT_VERSION
#endif // SIP_OS_WINDOWS	// by ysg
#endif // _STLPORT_VERSION

#ifdef SIP_COMP_VC6
	inline std::string toStringA(const uint &val) { return toString("%u", val); }
	inline std::basic_string<ucchar> toStringW(const uint &val) { return toString(L"%u", val); }

	inline std::string toStringA(const sint &val) { return toString("%d", val); }
	inline std::basic_string<ucchar> toStringW(const sint &val) { return toString(L"%d", val); }
#endif // SIP_COMP_VC6

template<class T>
void fromString(const std::string &str, T &obj)
{
	obj.fromString(str);
}

template<class T>
void fromString(const std::basic_string<ucchar> &str, T &obj)
{
	obj.fromString(str);
}

inline void fromString(const std::string &str, uint32 &val) { sscanf(str.c_str(), "%u", &val); }
inline void fromString(const std::string &str, sint32 &val) { sscanf(str.c_str(), "%d", &val); }
inline void fromString(const std::string &str, uint8 &val) { uint32 v; fromString(str, v); val = (uint8)v; }
inline void fromString(const std::string &str, sint8 &val) { sint32 v; fromString(str, v); val = (sint8)v; }
inline void fromString(const std::string &str, uint16 &val) { uint32 v; fromString(str, v); val = (uint16)v; }
inline void fromString(const std::string &str, sint16 &val) { uint32 v; fromString(str, v); val = (sint16)v; }
inline void fromString(const std::string &str, uint64 &val) { sscanf(str.c_str(), "%" SIP_I64 "u", &val); }
inline void fromString(const std::string &str, sint64 &val) { sscanf(str.c_str(), "%" SIP_I64 "d", &val); }
inline void fromString(const std::string &str, float &val) { sscanf(str.c_str(), "%f", &val); }
inline void fromString(const std::string &str, double &val) { sscanf(str.c_str(), "%lf", &val); }
inline void fromString(const std::string &str, bool &val) { uint32 v; fromString(str, v); val = (v==1); }
inline void fromString(const std::string &str, std::string &val) { val = str; }

inline void fromString(const std::basic_string<ucchar> &str, uint32 &val) { swscanf(str.c_str(), L"%u", &val); }
inline void fromString(const std::basic_string<ucchar> &str, sint32 &val) { swscanf(str.c_str(), L"%d", &val); }
inline void fromString(const std::basic_string<ucchar> &str, uint8 &val) { uint32 v; fromString(str, v); val = (uint8)v; }
inline void fromString(const std::basic_string<ucchar> &str, sint8 &val) { sint32 v; fromString(str, v); val = (sint8)v; }
inline void fromString(const std::basic_string<ucchar> &str, uint16 &val) { uint32 v; fromString(str, v); val = (uint16)v; }
inline void fromString(const std::basic_string<ucchar> &str, sint16 &val) { uint32 v; fromString(str, v); val = (sint16)v; }
inline void fromString(const std::basic_string<ucchar> &str, uint64 &val) { swscanf(str.c_str(), L"%I64u", &val); }
inline void fromString(const std::basic_string<ucchar> &str, sint64 &val) { swscanf(str.c_str(), L"%I64d", &val); }
inline void fromString(const std::basic_string<ucchar> &str, float &val) { swscanf(str.c_str(), L"%f", &val); }
inline void fromString(const std::basic_string<ucchar> &str, double &val) { swscanf(str.c_str(), L"%lf", &val); }
inline void fromString(const std::basic_string<ucchar> &str, bool &val) { uint32 v; fromString(str, v); val = (v==1); }
inline void fromString(const std::basic_string<ucchar> &str, std::basic_string<ucchar> &val) { val = str; }

#ifdef  SIP_OS_WINDOWS
inline void fromString(const std::string &str, uint &val) { sscanf(str.c_str(), "%u", &val); }
inline void fromString(const std::string &str, sint &val) { sscanf(str.c_str(), "%d", &val); }
inline void fromString(const std::basic_string<ucchar> &str, uint &val) { swscanf(str.c_str(), L"%u", &val); }
inline void fromString(const std::basic_string<ucchar> &str, sint &val) { swscanf(str.c_str(), L"%d", &val); }
#endif


// stl vectors of bool use bit reference and not real bools, so define the operator for bit reference

// Debug : Sept 01 2006
#ifdef _STLPORT_VERSION
#ifdef SIP_OS_WINDOWS	// by ysg
#if _STLPORT_VERSION >= 0x510
	inline void fromString(const std::string &str, std::priv::_Bit_reference &val) { uint32 v; fromString(str, v); val = (v==1); }
	inline void fromString(const std::basic_string<ucchar> &str, std::priv::_Bit_reference &val) { uint32 v; fromString(str, v); val = (v==1); }
#else
	inline void fromString(const std::string &str, std::_Bit_reference &val) { uint32 v; fromString(str, v); val = (v==1); }
	inline void fromString(const std::basic_string<ucchar> &str, std::_Bit_reference &val) { uint32 v; fromString(str, v); val = (v==1); }
#endif // _STLPORT_VERSION
#endif // SIP_OS_WINDOWS	// by ysg
#endif // _STLPORT_VERSION

#ifdef SIP_COMP_VC6
inline void fromString(const std::string &str, uint &val) { sscanf(str.c_str(), "%u", &val); }
inline void fromString(const std::string &str, sint &val) { sscanf(str.c_str(), "%d", &val); }

inline void fromString(const std::basic_string<ucchar> &str, uint &val) { swscanf(str.c_str(), L"%u", &val); }
inline void fromString(const std::basic_string<ucchar> &str, sint &val) { swscanf(str.c_str(), L"%d", &val); }
#endif // SIP_COMP_VC6

// #ifdef _UNICODE
#ifdef SIP_USE_UNICODE
#	define toStringT toStringW
#else
#	define toStringT toStringA
#endif
} // SIPBASE

#endif	// __STRING_COMMON_H__
