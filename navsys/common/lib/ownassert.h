#ifndef __OWNASSERT_H__
#define __OWNASSERT_H__

#ifdef ASSERT_ENABLED
#define assert(x, p) if (!(x)) __assert_handler((p));
#else
#define assert(x, p) ;
#endif

void __assert_handler(const char *p);

#endif
