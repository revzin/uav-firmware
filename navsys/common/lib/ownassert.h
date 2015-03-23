#ifndef __OWNASSERT_H__
#define __OWNASSERT_H__

#ifdef ASSERT_ENABLED
#define assert(x, s) if (!(x)) __assert_handler((s));
#else
#define assert(x, s) ;
#endif

void __assert_handler(const char *s);

#endif
