/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __LOG_H__
#define __LOG_H__

#include "types_sip.h"
#include "mutex.h"

#include <string>
#include <list>


namespace SIPBASE
{

class IDisplayer_base;

/**
 * When display() is called, the logger builds a string and sends it to its attached displayers.
 * The positive filters, if any, are applied first, then the negative filters.
 * See the sipdebug/sipinfo... macros in debug.h.
 *
 * \ref log_howto
 * \todo cado: display() and displayRaw() should save the string and send it only when displayRawNL()
 * (or a flush()-style method) is called.
 * \date 2001
 */

class ILog
{
public:

	typedef enum { LOG_NO=0, LOG_ERROR, LOG_WARNING, LOG_INFO, LOG_DEBUG, LOG_STAT, LOG_ASSERT, LOG_UNKNOWN } TLogType;

	ILog (TLogType logType = LOG_NO);

	/// Add a new displayer in the log. You have to create the displayer, remove it and delete it when you have finish with it.
	/// For example, in a 3dDisplayer, you can add the displayer when you want, and the displayer displays the string if the 3d
	/// screen is available and do nothing otherwise. In this case, if you want, you could leave the displayer all the time.
	void addDisplayer (IDisplayer_base *displayer, bool bypassFilter = false);

	/// Return the first displayer selected by his name
	IDisplayer_base *getDisplayer (const char *displayerName);

	/// Remove a displayer. If the displayer doesn't work in a specific time, you could remove it.
	void removeDisplayer (IDisplayer_base *displayer);

	/// Remove a displayer using his name
	void removeDisplayer (const char *displayerName);

	/// Returns true if the specified displayer is attached to the log object
	bool attached (IDisplayer_base *displayer) const;
	
	/// Returns true if no displayer is attached
	bool noDisplayer () const { return _Displayers.empty() && _BypassFilterDisplayers.empty(); }

	/// If !noDisplayer(), sets line and file parameters, and enters the mutex. If !noDisplayer(), don't forget to call display...() after, to release the mutex.
	void setPosition (sint line = -1, const char *fileName = NULL, const char *funcName = NULL);

protected:

	/// Symetric to setPosition(). Automatically called by display...(). Do not call if noDisplayer().
	void unsetPosition();

	typedef std::list<IDisplayer_base *>	CDisplayers;
	CDisplayers								_Displayers;
	CDisplayers								_BypassFilterDisplayers;	// these displayers always log info (by pass filter system)

	CMutex									_Mutex;
	uint32									_PosSet;

	TLogType								_LogType;
	const char								*_FileName;
	sint									_Line;
	const char								*_FuncName;
};

// ANSI version
class CLogA : public ILog
{
public:
	// Debug information
	struct TDisplayInfo
	{
		TDisplayInfo() : Date(0), LogType(ILog::LOG_NO), ThreadId(0), FileName(NULL), Line(-1), FuncName(NULL) {}
		
		time_t					Date;
		TLogType				LogType;
		std::basic_string<char>	ProcessName;
		uint					ThreadId;
		const char				*FileName;
		sint					Line;
		const char				*FuncName;
		std::basic_string<char>	CallstackAndLog;	// contains the callstack and a log with not filter of N last line (only in error/assert log type)
	};

	CLogA (TLogType logType = LOG_NO) : ILog(logType) { };

	/// Set the name of the process
	static void setProcessName (const std::basic_string<char> &processName);

	/// Find the process name if nobody call setProcessName before
	static void setDefaultProcessName ();

#ifdef SIP_OS_WINDOWS

#define CHECK_TYPES2(__a,__b) \
	inline __a(const char *fmt) { __b(fmt); } \
	template<class A> __a(const char *fmt, A a) { _check(a); __b(fmt, a); } \
	template<class A, class B> __a(const char *fmt, A a, B b) { _check(a); _check(b); __b(fmt, a, b); } \
	template<class A, class B, class C> __a(const char *fmt, A a, B b, C c) { _check(a); _check(b); _check(c); __b(fmt, a, b, c); } \
	template<class A, class B, class C, class D> __a(const char *fmt, A a, B b, C c, D d) { _check(a); _check(b); _check(c); _check(d); __b(fmt, a, b, c, d); } \
	template<class A, class B, class C, class D, class E> __a(const char *fmt, A a, B b, C c, D d, E e) { _check(a); _check(b); _check(c); _check(d); _check(e); __b(fmt, a, b, c, d, e); } \
	template<class A, class B, class C, class D, class E, class F> __a(const char *fmt, A a, B b, C c, D d, E e, F f) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); __b(fmt, a, b, c, d, e, f); } \
	template<class A, class B, class C, class D, class E, class F, class G> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); __b(fmt, a, b, c, d, e, f, g); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g, H h) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); __b(fmt, a, b, c, d, e, f, g, h); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); __b(fmt, a, b, c, d, e, f, g, h, i); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); __b(fmt, a, b, c, d, e, f, g, h, i, j); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); __b(fmt, a, b, c, d, e, f, g, h, i, j, k); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT, class U> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t, U u) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); _check(u); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT, class U, class V> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t, U u, V v) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); _check(u); _check(v); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT, class U, class V, class W> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t, U u, V v, W w) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); _check(u); _check(v); _check(w); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT, class U, class V, class W, class X> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t, U u, V v, W w, X x) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); _check(u); _check(v); _check(w); _check(x); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT, class U, class V, class W, class X, class Y> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t, U u, V v, W w, X x, Y y) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); _check(u); _check(v); _check(w); _check(x); _check(y); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT, class U, class V, class W, class X, class Y, class Z> __a(const char *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t, U u, V v, W w, X x, Y y, Z z) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); _check(u); _check(v); _check(w); _check(x); _check(y); _check(z); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z); }

	/// Display a string in decorated and final new line form to all attached displayers. Call setPosition() before. Releases the mutex.
	void _displayNL (const char *format, ...);
	CHECK_TYPES2(void displayNL, _displayNL)

	/// Display a string in decorated form to all attached displayers. Call setPosition() before. Releases the mutex.
	void _display (const char *format, ...);
	CHECK_TYPES2(void display, _display)
	
	/// Display a string with a final new line to all attached displayers. Call setPosition() before. Releases the mutex.
	void _displayRawNL (const char *format, ...);
	CHECK_TYPES2(void displayRawNL, _displayRawNL)
	
	/// Display a string (and nothing more) to all attached displayers. Call setPosition() before. Releases the mutex.
	void _displayRaw (const char *format, ...);
	CHECK_TYPES2(void displayRaw, _displayRaw)
	
	/// Display a raw text to the normal displayer but without filtering
	/// It's used by the Memdisplayer (little hack to work)
	void _forceDisplayRaw (const char *format, ...);
	CHECK_TYPES2(void forceDisplayRaw, _forceDisplayRaw)

#else

	/// Display a string in decorated and final new line form to all attached displayers. Call setPosition() before. Releases the mutex.
	void displayNL (const char *format, ...);

	/// Display a string in decorated form to all attached displayers. Call setPosition() before. Releases the mutex.
	void display (const char *format, ...);
	
	/// Display a string with a final new line to all attached displayers. Call setPosition() before. Releases the mutex.
	void displayRawNL (const char *format, ...);
	
	/// Display a string (and nothing more) to all attached displayers. Call setPosition() before. Releases the mutex.
	void displayRaw (const char *format, ...);
	
	/// Display a raw text to the normal displayer but without filtering
	/// It's used by the Memdisplayer (little hack to work)
	void forceDisplayRaw (const char *format, ...);

#endif

	/// Adds a positive filter. Tells the logger to log only the lines that contain filterstr
	void addPositiveFilter( const char *filterstr );

	/// Adds a negative filter. Tells the logger to discard the lines that contain filterstr
	void addNegativeFilter( const char *filterstr );

	/// Reset both filters
	void resetFilters();

	/// Removes a filter by name (in both filters).
	void removeFilter( const char *filterstr = NULL);

	/// Displays the list of filter into a log
	void displayFilter( CLogA &log );

protected:

	/// Returns true if the string must be logged, according to the current filter
	bool passFilter( const char *filter );

	/// Display a string in decorated form to all attached displayers.
	void displayString (const char *str);

	/// Display a Raw string to all attached displayers.
	void displayRawString (const char *str);

	std::list<std::basic_string<char> >		_NegativeFilter; /// "Discard" filter
	std::list<std::basic_string<char> >		_PositiveFilter; /// "Crop" filter

	static std::basic_string<char>			_ProcessName;

	std::basic_string<char>					TempString;
	TDisplayInfo							TempArgs;
};

// unicode version
class CLogW : public ILog
{
public:
	// Debug information
	struct TDisplayInfo
	{
		TDisplayInfo() : Date(0), LogType(ILog::LOG_NO), ThreadId(0), FileName(L""), Line(-1), FuncName(L"") {}
		
		time_t						Date;
		TLogType					LogType;
		std::basic_string<ucchar>	ProcessName;
		uint						ThreadId;
		std::basic_string<ucchar>	FileName;
		sint						Line;
		std::basic_string<ucchar>	FuncName;
		std::basic_string<ucchar>	CallstackAndLog;	// contains the callstack and a log with not filter of N last line (only in error/assert log type)
	};

	CLogW (TLogType logType = LOG_NO) : ILog(logType) { };

	/// Set the name of the process
	static void setProcessName  (const std::basic_string<char> &processName);
	static void setProcessNameW (const std::basic_string<ucchar> &processName);

	/// Find the process name if nobody call setProcessName before
	static void setDefaultProcessName ();

#ifdef SIP_OS_WINDOWS

#define CHECK_TYPES2_W(__a,__b) \
	inline __a(const ucchar *fmt) { __b(fmt); } \
	template<class A> __a(const ucchar *fmt, A a) { _check(a); __b(fmt, a); } \
	template<class A, class B> __a(const ucchar *fmt, A a, B b) { _check(a); _check(b); __b(fmt, a, b); } \
	template<class A, class B, class C> __a(const ucchar *fmt, A a, B b, C c) { _check(a); _check(b); _check(c); __b(fmt, a, b, c); } \
	template<class A, class B, class C, class D> __a(const ucchar *fmt, A a, B b, C c, D d) { _check(a); _check(b); _check(c); _check(d); __b(fmt, a, b, c, d); } \
	template<class A, class B, class C, class D, class E> __a(const ucchar *fmt, A a, B b, C c, D d, E e) { _check(a); _check(b); _check(c); _check(d); _check(e); __b(fmt, a, b, c, d, e); } \
	template<class A, class B, class C, class D, class E, class F> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); __b(fmt, a, b, c, d, e, f); } \
	template<class A, class B, class C, class D, class E, class F, class G> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); __b(fmt, a, b, c, d, e, f, g); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g, H h) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); __b(fmt, a, b, c, d, e, f, g, h); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); __b(fmt, a, b, c, d, e, f, g, h, i); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); __b(fmt, a, b, c, d, e, f, g, h, i, j); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); __b(fmt, a, b, c, d, e, f, g, h, i, j, k); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT, class U> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t, U u) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); _check(u); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT, class U, class V> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t, U u, V v) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); _check(u); _check(v); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT, class U, class V, class W> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t, U u, V v, W w) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); _check(u); _check(v); _check(w); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT, class U, class V, class W, class X> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t, U u, V v, W w, X x) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); _check(u); _check(v); _check(w); _check(x); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT, class U, class V, class W, class X, class Y> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t, U u, V v, W w, X x, Y y) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); _check(u); _check(v); _check(w); _check(x); _check(y); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y); } \
	template<class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R, class S, class TT, class U, class V, class W, class X, class Y, class Z> __a(const ucchar *fmt, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j, K k, L l, M m, N n, O o, P p, Q q, R r, S s, TT t, U u, V v, W w, X x, Y y, Z z) { _check(a); _check(b); _check(c); _check(d); _check(e); _check(f); _check(g); _check(h); _check(i); _check(j); _check(k); _check(l); _check(m); _check(n); _check(o); _check(p); _check(q); _check(r); _check(s); _check(t); _check(u); _check(v); _check(w); _check(x); _check(y); _check(z); __b(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z); }
	
	/// Display a string in decorated and final new line form to all attached displayers. Call setPosition() before. Releases the mutex.
	void _displayNL (const ucchar *format, ...);
	CHECK_TYPES2_W(void displayNL, _displayNL)

	void _displayNL (const char *format, ...);
	CHECK_TYPES2(void displayNL, _displayNL)

	/// Display a string in decorated form to all attached displayers. Call setPosition() before. Releases the mutex.
	void _display (const ucchar *format, ...);
	CHECK_TYPES2_W(void display, _display)
	
	void _display (const char *format, ...);
	CHECK_TYPES2(void display, _display)
	
	/// Display a string with a final new line to all attached displayers. Call setPosition() before. Releases the mutex.
	void _displayRawNL (const ucchar *format, ...);
	CHECK_TYPES2_W(void displayRawNL, _displayRawNL)

	void _displayRawNL (const char *format, ...);
	CHECK_TYPES2(void displayRawNL, _displayRawNL)
	
	/// Display a string (and nothing more) to all attached displayers. Call setPosition() before. Releases the mutex.
	void _displayRaw (const ucchar *format, ...);
	CHECK_TYPES2_W(void displayRaw, _displayRaw)

	void _displayRaw (const char *format, ...);
	CHECK_TYPES2(void displayRaw, _displayRaw)
	
	/// Display a raw text to the normal displayer but without filtering
	/// It's used by the Memdisplayer (little hack to work)
	void _forceDisplayRaw (const ucchar *format, ...);
	CHECK_TYPES2_W(void forceDisplayRaw, _forceDisplayRaw)

	void _forceDisplayRaw (const char *format, ...);
	CHECK_TYPES2(void forceDisplayRaw, _forceDisplayRaw)

#else

	/// Display a string in decorated and final new line form to all attached displayers. Call setPosition() before. Releases the mutex.
	void displayNL (const ucchar *format, ...);
	void displayNL (const char *format, ...);

	/// Display a string in decorated form to all attached displayers. Call setPosition() before. Releases the mutex.
	void display (const ucchar *format, ...);
	void display (const char *format, ...);
	
	/// Display a string with a final new line to all attached displayers. Call setPosition() before. Releases the mutex.
	void displayRawNL (const ucchar *format, ...);
	void displayRawNL (const char *format, ...);
	
	/// Display a string (and nothing more) to all attached displayers. Call setPosition() before. Releases the mutex.
	void displayRaw (const ucchar *format, ...);
	void displayRaw (const char *format, ...);
	
	/// Display a raw text to the normal displayer but without filtering
	/// It's used by the Memdisplayer (little hack to work)
	void forceDisplayRaw (const ucchar *format, ...);
	void forceDisplayRaw (const char *format, ...);

#endif

	/// Adds a positive filter. Tells the logger to log only the lines that contain filterstr
	void addPositiveFilter( const ucchar *filterstr );
	void addPositiveFilter( const char *filterstr );

	/// Adds a negative filter. Tells the logger to discard the lines that contain filterstr
	void addNegativeFilter( const ucchar *filterstr );
	void addNegativeFilter( const char *filterstr );

	/// Reset both filters
	void resetFilters();

	/// Removes a filter by name (in both filters).
	void removeFilter( const ucchar *filterstr = NULL);
	void removeFilter( const char *filterstr = NULL);

	/// Displays the list of filter into a log
	void displayFilter( CLogW &log );

protected:

	/// Returns true if the string must be logged, according to the current filter
	bool passFilter( const ucchar *filter );

	/// Display a string in decorated form to all attached displayers.
	void displayString (const ucchar *str);

	/// Display a Raw string to all attached displayers.
	void displayRawString (const ucchar *str);

	std::list<std::basic_string<ucchar> >		_NegativeFilter; /// "Discard" filter
	std::list<std::basic_string<ucchar> >		_PositiveFilter; /// "Crop" filter

	static std::basic_string<ucchar>			_ProcessName;

	std::basic_string<ucchar>					TempString;
	TDisplayInfo								TempArgs;
};
#define DEBUG_TO_LOG		TO_LOG(DebugLog)
#define INFO_TO_LOG			TO_LOG(InfoLog)
#define WARNING_TO_LOG		TO_LOG(WarningLog)

#define TO_LOG(log) if ( SIPBASE::g_bLog ) SIPBASE::createDebug(), SIPBASE::log->setPosition( __LINE__, __FILE__, __FUNCTION__ ), SIPBASE::log->displayNL


} // SIPBASE

#endif // __LOG_H__

/* End of log.h */
