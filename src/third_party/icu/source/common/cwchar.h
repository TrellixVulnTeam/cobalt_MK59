/*  
******************************************************************************
*
*   Copyright (C) 2001, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*   file name:  cwchar.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2001may25
*   created by: Markus W. Scherer
*
*   This file contains ICU-internal definitions of wchar_t operations.
*   These definitions were moved here from cstring.h so that fewer
*   ICU implementation files include wchar.h.
*/

#ifndef __CWCHAR_H__
#define __CWCHAR_H__

#if !defined(STARBOARD)
#include <string.h>
#include <stdlib.h>
#endif
#include "unicode/utypes.h"

/* Do this after utypes.h so that we have U_HAVE_WCHAR_H . */
#if !defined(STARBOARD)
#if U_HAVE_WCHAR_H
#   include <wchar.h>
#endif
#endif

/*===========================================================================*/
/* Wide-character functions                                                  */
/*===========================================================================*/

/* The following are not available on all systems, defined in wchar.h or string.h. */
#if defined(STARBOARD)
#   include "starboard/string.h"
#   define uprv_wcscpy SbStringCopyWide
#   define uprv_wcscat SbStringConcatWide
#   define uprv_wcslen SbStringGetLengthWide
#elif U_HAVE_WCSCPY
#   define uprv_wcscpy wcscpy
#   define uprv_wcscat wcscat
#   define uprv_wcslen wcslen
#else
U_CAPI wchar_t* U_EXPORT2 
uprv_wcscpy(wchar_t *dst, const wchar_t *src);
U_CAPI wchar_t* U_EXPORT2 
uprv_wcscat(wchar_t *dst, const wchar_t *src);
U_CAPI size_t U_EXPORT2 
uprv_wcslen(const wchar_t *src);
#endif

#if !defined(STARBOARD)  // Don't define these on Starboard as we do not have
                         // replacements for them yet.
/* The following are part of the ANSI C standard, defined in stdlib.h . */
#define uprv_wcstombs(mbstr, wcstr, count) U_STANDARD_CPP_NAMESPACE wcstombs(mbstr, wcstr, count)
#define uprv_mbstowcs(wcstr, mbstr, count) U_STANDARD_CPP_NAMESPACE mbstowcs(wcstr, mbstr, count)
#endif  // !defined(STARBOARD)

#endif
