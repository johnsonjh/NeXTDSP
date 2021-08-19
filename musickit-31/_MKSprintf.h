#include        <stdarg.h>
#include	<math.h>

/* These replace sprintf and vsprintf with thread-safe versions.
 *
 * Note that both of these return void here.
 */

extern void _MKSprintf(char *str, const char *fmt, ...);
extern void _MKVsprintf(char *str, const char *fmt, va_list ap);

