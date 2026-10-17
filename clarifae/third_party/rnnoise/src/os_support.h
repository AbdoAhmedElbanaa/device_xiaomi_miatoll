/* os_support.h — minimal shim for RNNoise release tarballs that omit it.
 * common.h already defines OPUS_INLINE/OPUS_CLEAR; this just satisfies the
 * #include and the few extra Opus helpers the SIMD path references. */
#ifndef OS_SUPPORT_H
#define OS_SUPPORT_H
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifndef OPUS_GNUC_PREREQ
# if defined(__GNUC__) && defined(__GNUC_MINOR__)
#  define OPUS_GNUC_PREREQ(maj,min) ((__GNUC__<<16)+__GNUC_MINOR__ >= ((maj)<<16)+(min))
# else
#  define OPUS_GNUC_PREREQ(maj,min) 0
# endif
#endif
#ifndef OPUS_INLINE
# define OPUS_INLINE inline
#endif
#ifndef OPUS_CLEAR
# define OPUS_CLEAR(dst,n) (memset((dst),0,(n)*sizeof(*(dst))))
#endif
#ifndef OPUS_COPY
# define OPUS_COPY(dst,src,n) (memcpy((dst),(src),(n)*sizeof(*(dst))))
#endif
#ifndef OPUS_MOVE
# define OPUS_MOVE(dst,src,n) (memmove((dst),(src),(n)*sizeof(*(dst))))
#endif
static OPUS_INLINE void *opus_alloc(size_t size){return malloc(size);}
static OPUS_INLINE void opus_free(void *ptr){free(ptr);}
#endif
