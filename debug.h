/*!
 * \file
 * <!--
 * Copyright 2003, 2004 Develer S.r.l. (http://www.develer.com/)
 * This file is part of DevLib - See devlib/README for information.
 * -->
 *
 * \brief Simple debug facilities for hosted and embedded C/C++ applications
 *
 * Debug output goes to stderr in hosted applications.  Freestanding (AKA embedded)
 * applications use drv/kdebug.c to output diagnostic messages to a serial terminal
 * or a JTAG debugger.
 *
 * \version $Id$
 * \author Bernardo Innocenti <bernie@develer.com>
 */

/*#*
 *#* $Log$
 *#* Revision 1.2  2004/09/14 21:01:46  bernie
 *#* Mark assertions as LIKELY().
 *#*
 *#* Revision 1.1  2004/09/14 20:19:47  bernie
 *#* Unified debug macros.
 *#*
 *#* Revision 1.3  2004/09/06 12:11:29  bernie
 *#* Merge debug.h with DevLib.
 *#*
 *#* Revision 1.2  2004/08/13 03:22:07  bernie
 *#* Resurrect some debug macros from old projects.
 *#*
 *#* Revision 1.1  2004/08/12 06:56:35  bernie
 *#* Move debug.h from device/ to util/.
 *#*
 *#* Revision 1.1  2004/08/12 06:52:26  bernie
 *#* Factor out a few simple debug macros.
 *#*
 *#*/
#ifndef DEVLIB_DEBUG_H
#define DEVLIB_DEBUG_H

/*
 * Try to guess whether we're running in a hosted or embedded environment.
 */
#ifndef OS_HOSTED
	#if defined(__unix__) || defined(_WIN32)
		#define OS_HOSTED  1
	#else
		#define OS_HOSTED  0
	#endif
#endif /* !OS_HOSTED */

#if OS_HOSTED
	/*
	 * For now, avoid dependency on compiler.h
	 */
	#ifndef FORMAT
	#define FORMAT(x,y,z) /* nothing */
	#endif
	#ifndef INLINE
	#define INLINE static inline
	#endif
	#ifndef UNUSED
	#define UNUSED(type,name) type
	#endif
#else /* !OS_HOSTED */
	#include <compiler.h>
#endif /* !OS_HOSTED */

/*!
 * \def _DEBUG
 *
 * This preprocessor symbol is defined only in debug builds.
 *
 * The build infrastructure must arrange for _DEBUG to
 * be prepredefined for all source files being compiled.
 *
 * This is compatible with the Microsoft convention for
 * the default Debug and Release targets.
 */
#ifdef _DEBUG

	// STLport specific: enable extra checks
	#define __STL_DEBUG 1

	// MSVC specific: Enable memory allocation debug
	#if defined(_MSC_VER)
		#include <crtdbg.h>
	#endif

	/*
	 * On UNIX systems the extabilished practice is to define
	 * NDEBUG for release builds and nothing for debug builds.
	 */
	#ifdef NDEBUG
	#undef NDEBUG
	#endif

	/*!
	 * This macro duplicates the old MSVC trick of redefining
	 * THIS_FILE locally to avoid the overhead of many duplicate
	 * strings in ASSERT().
	 */
	#ifndef THIS_FILE
		#define THIS_FILE  __FILE__
	#endif

	/*!
	 * This macro can be used to conditionally exclude one or more
	 * statements conditioned on \c _DEBUG, avoiding the clutter
	 * of #ifdef/#endif pairs.
	 *
	 * \example
	 *     struct FooBar
	 *     {
	 *         int foo;
	 *         bool bar;
	 *         DB(int ref_count;) // Track number of users
	 *
	 *         void release()
	 *         {
         *             DB(--ref_count;)
	 *         }
	 *     };
	 *
	 * \endexample
	 */
	#define DB(x) x

	#if OS_HOSTED
		#include <stdio.h>
		#include <stdarg.h>
		INLINE void kdbg_init(void) { /* nop */ }
		INLINE void kputchar(char c)
		{
			putc(c, stderr);
		}
		INLINE void kputs(const char *str)
		{
			fputs(str, stderr);
		}
		INLINE void kprintf(const char * fmt, ...) FORMAT(__printf__, 1, 2)
		{
			va_list ap;
			va_start(ap, fmt);
			vfprintf(stderr, fmt, ap);
			va_end(ap);
		}
		void kdump(const void *buf, size_t len); /* UNIMPLEMENTED */

		#ifndef ASSERT
			#include <assert.h>
			#define ASSERT(x) assert(x)
		#endif /* ASSERT */

		/*!
		 * Check that the given pointer is not NULL or pointing to raw memory.
		 *
		 * The assumption here is that valid pointers never point to low
		 * memory regions.  This helps catching pointers taken from
		 * struct/class memebers when the struct pointer was NULL.
		 */
		#define ASSERT_VALID_PTR(p)  ASSERT((unsigned long)(p) > 0x200)

		#define ASSERT_VALID_PTR_OR_NULL(p)  ASSERT((((p) == NULL) || ((unsigned long)(p) >= 0x200)))
	#else /* !OS_HOSTED */

		/* These are implemented in drv/kdebug.c */
		void kdbg_init(void);
		void kputchar(char c);
		void kdump(const void *buf, size_t len);

		#ifdef __AVR__
			#include <avr/pgmspace.h>
			void kputs_P(const char *PROGMEM str);
			void kprintf_P(const char *PROGMEM fmt, ...) FORMAT(__printf__, 1, 2);
			int __assert_P(const char *PROGMEM cond, const char *PROGMEM file, int line);
			int __invalid_ptr_P(void *p, const char *PROGMEM name, const char *PROGMEM file, int line);
			#define kputs(str)  kputs_P(PSTR(str))
			#define kprintf(fmt, ...)  kprintf_P(PSTR(fmt) ,## __VA_ARGS__)
			#define __assert(cond, file, line)  __assert_P(PSTR(cond), PSTR(file), (line))
			#define __invalid_ptr(p, name, file, line)  __invalid_ptr_P((p), PSTR(name), PSTR(file), (line))
		#else /* !__AVR__ */
			void kputs(const char *str);
			void kprintf(const char * fmt, ...) FORMAT(__printf__, 1, 2);
			int __assert(const char *cond, const char *file, int line);
			int __invalid_ptr(void *p, const char *name, const char *file, int line);
		#endif /* !__AVR__ */

		void __init_wall(long *wall, int size);
		int __check_wall(long *wall, int size, const char *name, const char *file, int line);

		#ifndef CONFIG_KDEBUG_ASSERT_NO_TEXT
			#define ASSERT(x)         (LIKELY(x) ? 0 : __assert(#x, THIS_FILE, __LINE__))
			#define ASSERT2(x, help)  (LIKELY(x) ? 0 : __assert(help " (" #x ")", THIS_FILE, __LINE__))
		#else
			#define ASSERT(x)         (LIKELY(x) ? 0 : __assert("", THIS_FILE, __LINE__))
			#define ASSERT2(x, help)  ASSERT(x)
		#endif

		#define ASSERT_VALID_PTR(p)         (LIKELY((p) >= 0x200) ? 0 : __invalid_ptr(p, #p, THIS_FILE, __LINE__))
		#define ASSERT_VALID_PTR_OR_NULL(p) (LIKELY((p == NULL) || ((p) >= 0x200)) ? 0 : __invalid_ptr((p), #p, THIS_FILE, __LINE__))
		#define TRACE                       kprintf("%s()\n", __FUNCTION__)
		#define TRACEMSG(msg,...)           kprintf("%s(): " msg "\n", __FUNCTION__, ## __VA_ARGS__)

	#endif /* !OS_HOSTED */

	/*!
	 * \name Walls to detect data corruption
	 * \{
	 */
	#define WALL_SIZE                    8
	#define WALL_VALUE                   (long)0xABADCAFEL
	#define DECLARE_WALL(name,size)      long name[(size) / sizeof(long)];
	#define FWD_DECLARE_WALL(name,size)  extern long name[(size) / sizeof(long)];
	#define INIT_WALL(name)              __init_wall((name), countof(name))
	#define CHECK_WALL(name)             __check_wall((name), countof(name), #name, THIS_FILE, __LINE__)
	/*\}*/

	/*!
	 * Check that the given pointer actually points to an object
	 * of the specified type.
	 */
	#define ASSERT_VALID_OBJ(_t, _o) do { \
		ASSERT_VALID_PTR((_o)); \
		ASSERT(dynamic_cast<_t>((_o)) != NULL); \
	}

	/*!
	 * \name Debug object creation and destruction.
	 *
	 * These macros help track some kinds of leaks in C++ programs.
	 * Usage is as follows:
	 *
	 * \example
	 *   class Foo
	 *   {
	 *       DECLARE_INSTANCE_TRACKING(Foo)
	 *
	 *       Foo()
	 *       {
	 *           NEW_INSTANCE(Foo);
	 *           // ...
	 *       }
	 *
	 *       ~Foo()
	 *       {
	 *           DELETE_INSTANCE(Foo);
	 *           // ...
	 *       }
	 *   };
	 *
	 *   // Put this in the implementation file of the class
	 *   IMPLEMENT_INSTANCE_TRACKING(Foo)
	 *
	 *   // Client code
	 *   int main(void)
	 *   {
	 *        Foo *foo = new Foo;
	 *        cout << GET_INSTANCE_COUNT(Foo) << endl; // prints "1"
	 *        delete foo;
	 *        ASSERT_ZERO_INSTANCES(Foo); // OK
	 *   }
	 *
	 * \end example
	 */
	#define NEW_INSTANCE(CLASS)                do { ++CLASS::__instances } while (0)
	#define DELETE_INSTANCE(CLASS)             do { --CLASS::__instances } while (0)
	#define ASSERT_ZERO_INSTANCES(CLASS)       ASSERT(CLASS::__instances == 0)
	#define GET_INSTANCE_COUNT(CLASS)          (CLASS::__instances)
	#define DECLARE_INSTANCE_TRACKING(CLASS)   static int __instances
	#define IMPLEMENT_INSTANCE_TRACKING(CLASS) int CLASS::__instances = 0

#else /* !_DEBUG */

	/*
	 * On UNIX systems the extabilished practice is to define
	 * NDEBUG for release builds and nothing for debug builds.
	 */
	#ifndef NDEBUG
	#define NDEBUG 1
	#endif

	#define DB(x)  /* nothing */
	#ifndef ASSERT
		#define ASSERT(x)  do {} while (0)
	#endif /* ASSERT */
	#define ASSERT2(x, help)             do {} while (0)
	#define ASSERT_VALID_PTR(p)          do {} while (0)
	#define ASSERT_VALID_PTR_OR_NULL(p)  do {} while (0)
	#define ASSERT_VALID_OBJ(_t, _o)     do {} while (0)
	#define TRACE                        do {} while (0)
	#define TRACEMSG(x,...)              do {} while (0)

	#define DECLARE_WALL(name, size)     /* nothing */
	#define FWD_DECLARE_WALL(name, size) /* nothing */
	#define INIT_WALL(name)              do {} while (0)
	#define CHECK_WALL(name)             do {} while (0)

	#define NEW_INSTANCE(CLASS)                do {} while (0)
	#define DELETE_INSTANCE(CLASS)             do {} while (0)
	#define ASSERT_ZERO_INSTANCES(CLASS)       do {} while (0)
	#define GET_INSTANCE_COUNT(CLASS)          ERROR_ONLY_FOR_DEBUG
	#define DECLARE_INSTANCE_TRACKING(CLASS)
	#define IMPLEMENT_INSTANCE_TRACKING(CLASS)

	INLINE void kdbg_init(void) { /* nop */ }
	INLINE void kputchar(UNUSED(char, c)) { /* nop */ }
	INLINE void kputs(UNUSED(const char*, str)) { /* nop */ }
	INLINE void kprintf(UNUSED(const char*, fmt), ...) { /* nop */ }

#endif /* _DEBUG */

#endif /* DEVLIB_DEBUG_H */
