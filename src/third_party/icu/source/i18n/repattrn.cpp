//
//  file:  repattrn.cpp
//
/*
***************************************************************************
*   Copyright (C) 2002-2015 International Business Machines Corporation   *
*   and others. All rights reserved.                                      *
***************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_REGULAR_EXPRESSIONS
#include "starboard/client_porting/poem/assert_poem.h"
#include "starboard/client_porting/poem/string_poem.h"

#include "unicode/regex.h"
#include "unicode/uclean.h"
#include "uassert.h"
#include "uhash.h"
#include "uvector.h"
#include "uvectr32.h"
#include "uvectr64.h"
#include "regexcmp.h"
#include "regeximp.h"
#include "regexst.h"

U_NAMESPACE_BEGIN

//--------------------------------------------------------------------------
//
//    RegexPattern    Default Constructor
//
//--------------------------------------------------------------------------
RegexPattern::RegexPattern() {
    // Init all of this instances data.
    init();
}


//--------------------------------------------------------------------------
//
//   Copy Constructor        Note:  This is a rather inefficient implementation,
//                                  but it probably doesn't matter.
//
//--------------------------------------------------------------------------
RegexPattern::RegexPattern(const RegexPattern &other) :  UObject(other) {
    init();
    *this = other;
}



//--------------------------------------------------------------------------
//
//    Assignment Operator
//
//--------------------------------------------------------------------------
RegexPattern &RegexPattern::operator = (const RegexPattern &other) {
    if (this == &other) {
        // Source and destination are the same.  Don't do anything.
        return *this;
    }

    // Clean out any previous contents of object being assigned to.
    zap();

    // Give target object a default initialization
    init();

    // Copy simple fields
    fDeferredStatus   = other.fDeferredStatus;

    if (U_FAILURE(fDeferredStatus)) {
        return *this;
    }

    if (other.fPatternString == NULL) {
        fPatternString = NULL;
        fPattern = utext_clone(fPattern, other.fPattern, FALSE, TRUE, &fDeferredStatus);
    } else {
        fPatternString = new UnicodeString(*(other.fPatternString));
        if (fPatternString == NULL) {
            fDeferredStatus = U_MEMORY_ALLOCATION_ERROR;
        } else {
            fPattern = utext_openConstUnicodeString(NULL, fPatternString, &fDeferredStatus);
        }
    }
    if (U_FAILURE(fDeferredStatus)) {
        return *this;
    }

    fFlags            = other.fFlags;
    fLiteralText      = other.fLiteralText;
    fMinMatchLen      = other.fMinMatchLen;
    fFrameSize        = other.fFrameSize;
    fDataSize         = other.fDataSize;
    fStaticSets       = other.fStaticSets;
    fStaticSets8      = other.fStaticSets8;

    fStartType        = other.fStartType;
    fInitialStringIdx = other.fInitialStringIdx;
    fInitialStringLen = other.fInitialStringLen;
    *fInitialChars    = *other.fInitialChars;
    fInitialChar      = other.fInitialChar;
    *fInitialChars8   = *other.fInitialChars8;
    fNeedsAltInput    = other.fNeedsAltInput;

    //  Copy the pattern.  It's just values, nothing deep to copy.
    fCompiledPat->assign(*other.fCompiledPat, fDeferredStatus);
    fGroupMap->assign(*other.fGroupMap, fDeferredStatus);

    //  Copy the Unicode Sets.
    //    Could be made more efficient if the sets were reference counted and shared,
    //    but I doubt that pattern copying will be particularly common.
    //    Note:  init() already added an empty element zero to fSets
    int32_t i;
    int32_t  numSets = other.fSets->size();
    fSets8 = new Regex8BitSet[numSets];
    if (fSets8 == NULL) {
    	fDeferredStatus = U_MEMORY_ALLOCATION_ERROR;
    	return *this;
    }
    for (i=1; i<numSets; i++) {
        if (U_FAILURE(fDeferredStatus)) {
            return *this;
        }
        UnicodeSet *sourceSet = (UnicodeSet *)other.fSets->elementAt(i);
        UnicodeSet *newSet    = new UnicodeSet(*sourceSet);
        if (newSet == NULL) {
            fDeferredStatus = U_MEMORY_ALLOCATION_ERROR;
            break;
        }
        fSets->addElement(newSet, fDeferredStatus);
        fSets8[i] = other.fSets8[i];
    }

    // Copy the named capture group hash map.
    int32_t hashPos = UHASH_FIRST;
    while (const UHashElement *hashEl = uhash_nextElement(other.fNamedCaptureMap, &hashPos)) {
        if (U_FAILURE(fDeferredStatus)) {
            break;
        }
        const UnicodeString *name = (const UnicodeString *)hashEl->key.pointer;
        UnicodeString *key = new UnicodeString(*name);
        int32_t val = hashEl->value.integer;
        if (key == NULL) {
            fDeferredStatus = U_MEMORY_ALLOCATION_ERROR;
        } else {
            uhash_puti(fNamedCaptureMap, key, val, &fDeferredStatus);
        }
    }
    return *this;
}


//--------------------------------------------------------------------------
//
//    init        Shared initialization for use by constructors.
//                Bring an uninitialized RegexPattern up to a default state.
//
//--------------------------------------------------------------------------
void RegexPattern::init() {
    fFlags            = 0;
    fCompiledPat      = 0;
    fLiteralText.remove();
    fSets             = NULL;
    fSets8            = NULL;
    fDeferredStatus   = U_ZERO_ERROR;
    fMinMatchLen      = 0;
    fFrameSize        = 0;
    fDataSize         = 0;
    fGroupMap         = NULL;
    fStaticSets       = NULL;
    fStaticSets8      = NULL;
    fStartType        = START_NO_INFO;
    fInitialStringIdx = 0;
    fInitialStringLen = 0;
    fInitialChars     = NULL;
    fInitialChar      = 0;
    fInitialChars8    = NULL;
    fNeedsAltInput    = FALSE;
    fNamedCaptureMap  = NULL;

    fPattern          = NULL; // will be set later
    fPatternString    = NULL; // may be set later
    fCompiledPat      = new UVector64(fDeferredStatus);
    fGroupMap         = new UVector32(fDeferredStatus);
    fSets             = new UVector(fDeferredStatus);
    fInitialChars     = new UnicodeSet;
    fInitialChars8    = new Regex8BitSet;
    fNamedCaptureMap  = uhash_open(uhash_hashUnicodeString,     // Key hash function
                                   uhash_compareUnicodeString,  // Key comparator function
                                   uhash_compareLong,           // Value comparator function
                                   &fDeferredStatus);
    if (U_FAILURE(fDeferredStatus)) {
        return;
    }
    if (fCompiledPat == NULL  || fGroupMap == NULL || fSets == NULL ||
            fInitialChars == NULL || fInitialChars8 == NULL || fNamedCaptureMap == NULL) {
        fDeferredStatus = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    // Slot zero of the vector of sets is reserved.  Fill it here.
    fSets->addElement((int32_t)0, fDeferredStatus);

    // fNamedCaptureMap owns its key strings, type (UnicodeString *)
    uhash_setKeyDeleter(fNamedCaptureMap, uprv_deleteUObject);
}


//--------------------------------------------------------------------------
//
//   zap            Delete everything owned by this RegexPattern.
//
//--------------------------------------------------------------------------
void RegexPattern::zap() {
    delete fCompiledPat;
    fCompiledPat = NULL;
    int i;
    for (i=1; i<fSets->size(); i++) {
        UnicodeSet *s;
        s = (UnicodeSet *)fSets->elementAt(i);
        if (s != NULL) {
            delete s;
        }
    }
    delete fSets;
    fSets = NULL;
    delete[] fSets8;
    fSets8 = NULL;
    delete fGroupMap;
    fGroupMap = NULL;
    delete fInitialChars;
    fInitialChars = NULL;
    delete fInitialChars8;
    fInitialChars8 = NULL;
    if (fPattern != NULL) {
        utext_close(fPattern);
        fPattern = NULL;
    }
    if (fPatternString != NULL) {
        delete fPatternString;
        fPatternString = NULL;
    }
    uhash_close(fNamedCaptureMap);
    fNamedCaptureMap = NULL;
}


//--------------------------------------------------------------------------
//
//   Destructor
//
//--------------------------------------------------------------------------
RegexPattern::~RegexPattern() {
    zap();
}


//--------------------------------------------------------------------------
//
//   Clone
//
//--------------------------------------------------------------------------
RegexPattern  *RegexPattern::clone() const {
    RegexPattern  *copy = new RegexPattern(*this);
    return copy;
}


//--------------------------------------------------------------------------
//
//   operator ==   (comparison)    Consider to patterns to be == if the
//                                 pattern strings and the flags are the same.
//                                 Note that pattern strings with the same
//                                 characters can still be considered different.
//
//--------------------------------------------------------------------------
UBool   RegexPattern::operator ==(const RegexPattern &other) const {
    if (this->fFlags == other.fFlags && this->fDeferredStatus == other.fDeferredStatus) {
        if (this->fPatternString != NULL && other.fPatternString != NULL) {
            return *(this->fPatternString) == *(other.fPatternString);
        } else if (this->fPattern == NULL) {
            if (other.fPattern == NULL) {
                return TRUE;
            }
        } else if (other.fPattern != NULL) {
            UTEXT_SETNATIVEINDEX(this->fPattern, 0);
            UTEXT_SETNATIVEINDEX(other.fPattern, 0);
            return utext_equals(this->fPattern, other.fPattern);
        }
    }
    return FALSE;
}

//---------------------------------------------------------------------
//
//   compile
//
//---------------------------------------------------------------------
RegexPattern * U_EXPORT2
RegexPattern::compile(const UnicodeString &regex,
                      uint32_t             flags,
                      UParseError          &pe,
                      UErrorCode           &status)
{
    if (U_FAILURE(status)) {
        return NULL;
    }

    const uint32_t allFlags = UREGEX_CANON_EQ | UREGEX_CASE_INSENSITIVE | UREGEX_COMMENTS |
    UREGEX_DOTALL   | UREGEX_MULTILINE        | UREGEX_UWORD |
    UREGEX_ERROR_ON_UNKNOWN_ESCAPES           | UREGEX_UNIX_LINES | UREGEX_LITERAL;

    if ((flags & ~allFlags) != 0) {
        status = U_REGEX_INVALID_FLAG;
        return NULL;
    }

    if ((flags & UREGEX_CANON_EQ) != 0) {
        status = U_REGEX_UNIMPLEMENTED;
        return NULL;
    }

    RegexPattern *This = new RegexPattern;
    if (This == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    if (U_FAILURE(This->fDeferredStatus)) {
        status = This->fDeferredStatus;
        delete This;
        return NULL;
    }
    This->fFlags = flags;

    RegexCompile     compiler(This, status);
    compiler.compile(regex, pe, status);

    if (U_FAILURE(status)) {
        delete This;
        This = NULL;
    }

    return This;
}


//
//   compile, UText mode
//
RegexPattern * U_EXPORT2
RegexPattern::compile(UText                *regex,
                      uint32_t             flags,
                      UParseError          &pe,
                      UErrorCode           &status)
{
    if (U_FAILURE(status)) {
        return NULL;
    }

    const uint32_t allFlags = UREGEX_CANON_EQ | UREGEX_CASE_INSENSITIVE | UREGEX_COMMENTS |
                              UREGEX_DOTALL   | UREGEX_MULTILINE        | UREGEX_UWORD |
                              UREGEX_ERROR_ON_UNKNOWN_ESCAPES           | UREGEX_UNIX_LINES | UREGEX_LITERAL;

    if ((flags & ~allFlags) != 0) {
        status = U_REGEX_INVALID_FLAG;
        return NULL;
    }

    if ((flags & UREGEX_CANON_EQ) != 0) {
        status = U_REGEX_UNIMPLEMENTED;
        return NULL;
    }

    RegexPattern *This = new RegexPattern;
    if (This == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    if (U_FAILURE(This->fDeferredStatus)) {
        status = This->fDeferredStatus;
        delete This;
        return NULL;
    }
    This->fFlags = flags;

    RegexCompile     compiler(This, status);
    compiler.compile(regex, pe, status);

    if (U_FAILURE(status)) {
        delete This;
        This = NULL;
    }

    return This;
}

//
//   compile with default flags.
//
RegexPattern * U_EXPORT2
RegexPattern::compile(const UnicodeString &regex,
                      UParseError         &pe,
                      UErrorCode          &err)
{
    return compile(regex, 0, pe, err);
}


//
//   compile with default flags, UText mode
//
RegexPattern * U_EXPORT2
RegexPattern::compile(UText               *regex,
                      UParseError         &pe,
                      UErrorCode          &err)
{
    return compile(regex, 0, pe, err);
}


//
//   compile with no UParseErr parameter.
//
RegexPattern * U_EXPORT2
RegexPattern::compile(const UnicodeString &regex,
                      uint32_t             flags,
                      UErrorCode          &err)
{
    UParseError pe;
    return compile(regex, flags, pe, err);
}


//
//   compile with no UParseErr parameter, UText mode
//
RegexPattern * U_EXPORT2
RegexPattern::compile(UText                *regex,
                      uint32_t             flags,
                      UErrorCode           &err)
{
    UParseError pe;
    return compile(regex, flags, pe, err);
}


//---------------------------------------------------------------------
//
//   flags
//
//---------------------------------------------------------------------
uint32_t RegexPattern::flags() const {
    return fFlags;
}


//---------------------------------------------------------------------
//
//   matcher(UnicodeString, err)
//
//---------------------------------------------------------------------
RegexMatcher *RegexPattern::matcher(const UnicodeString &input,
                                    UErrorCode          &status)  const {
    RegexMatcher    *retMatcher = matcher(status);
    if (retMatcher != NULL) {
        retMatcher->fDeferredStatus = status;
        retMatcher->reset(input);
    }
    return retMatcher;
}


//---------------------------------------------------------------------
//
//   matcher(status)
//
//---------------------------------------------------------------------
RegexMatcher *RegexPattern::matcher(UErrorCode &status)  const {
    RegexMatcher    *retMatcher = NULL;

    if (U_FAILURE(status)) {
        return NULL;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return NULL;
    }

    retMatcher = new RegexMatcher(this);
    if (retMatcher == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    return retMatcher;
}



//---------------------------------------------------------------------
//
//   matches        Convenience function to test for a match, starting
//                  with a pattern string and a data string.
//
//---------------------------------------------------------------------
UBool U_EXPORT2 RegexPattern::matches(const UnicodeString   &regex,
              const UnicodeString   &input,
                    UParseError     &pe,
                    UErrorCode      &status) {

    if (U_FAILURE(status)) {return FALSE;}

    UBool         retVal;
    RegexPattern *pat     = NULL;
    RegexMatcher *matcher = NULL;

    pat     = RegexPattern::compile(regex, 0, pe, status);
    matcher = pat->matcher(input, status);
    retVal  = matcher->matches(status);

    delete matcher;
    delete pat;
    return retVal;
}


//
//   matches, UText mode
//
UBool U_EXPORT2 RegexPattern::matches(UText                *regex,
                    UText           *input,
                    UParseError     &pe,
                    UErrorCode      &status) {

    if (U_FAILURE(status)) {return FALSE;}

    UBool         retVal  = FALSE;
    RegexPattern *pat     = NULL;
    RegexMatcher *matcher = NULL;

    pat     = RegexPattern::compile(regex, 0, pe, status);
    matcher = pat->matcher(status);
    if (U_SUCCESS(status)) {
        matcher->reset(input);
        retVal  = matcher->matches(status);
    }

    delete matcher;
    delete pat;
    return retVal;
}





//---------------------------------------------------------------------
//
//   pattern
//
//---------------------------------------------------------------------
UnicodeString RegexPattern::pattern() const {
    if (fPatternString != NULL) {
        return *fPatternString;
    } else if (fPattern == NULL) {
        return UnicodeString();
    } else {
        UErrorCode status = U_ZERO_ERROR;
        int64_t nativeLen = utext_nativeLength(fPattern);
        int32_t len16 = utext_extract(fPattern, 0, nativeLen, NULL, 0, &status); // buffer overflow error
        UnicodeString result;

        status = U_ZERO_ERROR;
        UChar *resultChars = result.getBuffer(len16);
        utext_extract(fPattern, 0, nativeLen, resultChars, len16, &status); // unterminated warning
        result.releaseBuffer(len16);

        return result;
    }
}




//---------------------------------------------------------------------
//
//   patternText
//
//---------------------------------------------------------------------
UText *RegexPattern::patternText(UErrorCode      &status) const {
    if (U_FAILURE(status)) {return NULL;}
    status = U_ZERO_ERROR;

    if (fPattern != NULL) {
        return fPattern;
    } else {
        RegexStaticSets::initGlobals(&status);
        return RegexStaticSets::gStaticSets->fEmptyText;
    }
}


//--------------------------------------------------------------------------------
//
//  groupNumberFromName()
//
//--------------------------------------------------------------------------------
int32_t RegexPattern::groupNumberFromName(const UnicodeString &groupName, UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return 0;
    }

    // No need to explicitly check for syntactically valid names.
    // Invalid ones will never be in the map, and the lookup will fail.

    int32_t number = uhash_geti(fNamedCaptureMap, &groupName);
    if (number == 0) {
        status = U_REGEX_INVALID_CAPTURE_GROUP_NAME;
    }
    return number;
}

int32_t RegexPattern::groupNumberFromName(const char *groupName, int32_t nameLength, UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return 0;
    }
    UnicodeString name(groupName, nameLength, US_INV);
    return groupNumberFromName(name, status);
}


//---------------------------------------------------------------------
//
//   split
//
//---------------------------------------------------------------------
int32_t  RegexPattern::split(const UnicodeString &input,
        UnicodeString    dest[],
        int32_t          destCapacity,
        UErrorCode      &status) const
{
    if (U_FAILURE(status)) {
        return 0;
    };

    RegexMatcher  m(this);
    int32_t r = 0;
    // Check m's status to make sure all is ok.
    if (U_SUCCESS(m.fDeferredStatus)) {
    	r = m.split(input, dest, destCapacity, status);
    }
    return r;
}

//
//   split, UText mode
//
int32_t  RegexPattern::split(UText *input,
        UText           *dest[],
        int32_t          destCapacity,
        UErrorCode      &status) const
{
    if (U_FAILURE(status)) {
        return 0;
    };

    RegexMatcher  m(this);
    int32_t r = 0;
    // Check m's status to make sure all is ok.
    if (U_SUCCESS(m.fDeferredStatus)) {
    	r = m.split(input, dest, destCapacity, status);
    }
    return r;
}



//---------------------------------------------------------------------
//
//   dump    Output the compiled form of the pattern.
//           Debugging function only.
//
//---------------------------------------------------------------------
void   RegexPattern::dumpOp(int32_t index) const {
    (void)index;  // Suppress warnings in non-debug build.
#if defined(REGEX_DEBUG)
    static const char * const opNames[] = {URX_OPCODE_NAMES};
    int32_t op          = fCompiledPat->elementAti(index);
    int32_t val         = URX_VAL(op);
    int32_t type        = URX_TYPE(op);
    int32_t pinnedType  = type;
    if ((uint32_t)pinnedType >= sizeof(opNames)/sizeof(char *)) {
        pinnedType = 0;
    }

    printf("%4d   %08x    %-15s  ", index, op, opNames[pinnedType]);
    switch (type) {
    case URX_NOP:
    case URX_DOTANY:
    case URX_DOTANY_ALL:
    case URX_FAIL:
    case URX_CARET:
    case URX_DOLLAR:
    case URX_BACKSLASH_G:
    case URX_BACKSLASH_X:
    case URX_END:
    case URX_DOLLAR_M:
    case URX_CARET_M:
        // Types with no operand field of interest.
        break;

    case URX_RESERVED_OP:
    case URX_START_CAPTURE:
    case URX_END_CAPTURE:
    case URX_STATE_SAVE:
    case URX_JMP:
    case URX_JMP_SAV:
    case URX_JMP_SAV_X:
    case URX_BACKSLASH_B:
    case URX_BACKSLASH_BU:
    case URX_BACKSLASH_D:
    case URX_BACKSLASH_Z:
    case URX_STRING_LEN:
    case URX_CTR_INIT:
    case URX_CTR_INIT_NG:
    case URX_CTR_LOOP:
    case URX_CTR_LOOP_NG:
    case URX_RELOC_OPRND:
    case URX_STO_SP:
    case URX_LD_SP:
    case URX_BACKREF:
    case URX_STO_INP_LOC:
    case URX_JMPX:
    case URX_LA_START:
    case URX_LA_END:
    case URX_BACKREF_I:
    case URX_LB_START:
    case URX_LB_CONT:
    case URX_LB_END:
    case URX_LBN_CONT:
    case URX_LBN_END:
    case URX_LOOP_C:
    case URX_LOOP_DOT_I:
    case URX_BACKSLASH_H:
    case URX_BACKSLASH_R:
    case URX_BACKSLASH_V:
        // types with an integer operand field.
        printf("%d", val);
        break;

    case URX_ONECHAR:
    case URX_ONECHAR_I:
        printf("%c", val<256?val:'?');
        break;

    case URX_STRING:
    case URX_STRING_I:
        {
            int32_t lengthOp       = fCompiledPat->elementAti(index+1);
            U_ASSERT(URX_TYPE(lengthOp) == URX_STRING_LEN);
            int32_t length = URX_VAL(lengthOp);
            int32_t i;
            for (i=val; i<val+length; i++) {
                UChar c = fLiteralText[i];
                if (c < 32 || c >= 256) {c = '.';}
                printf("%c", c);
            }
        }
        break;

    case URX_SETREF:
    case URX_LOOP_SR_I:
        {
            UnicodeString s;
            UnicodeSet *set = (UnicodeSet *)fSets->elementAt(val);
            set->toPattern(s, TRUE);
            for (int32_t i=0; i<s.length(); i++) {
                printf("%c", s.charAt(i));
            }
        }
        break;

    case URX_STATIC_SETREF:
    case URX_STAT_SETREF_N:
        {
            UnicodeString s;
            if (val & URX_NEG_SET) {
                printf("NOT ");
                val &= ~URX_NEG_SET;
            }
            UnicodeSet *set = fStaticSets[val];
            set->toPattern(s, TRUE);
            for (int32_t i=0; i<s.length(); i++) {
                printf("%c", s.charAt(i));
            }
        }
        break;


    default:
        printf("??????");
        break;
    }
    printf("\n");
#endif
}


void RegexPattern::dumpPattern() const {
#if defined(REGEX_DEBUG)
    // TODO: This function assumes an ASCII based charset.
    int      index;
    int      i;

    printf("Original Pattern:  ");
    UChar32 c = utext_next32From(fPattern, 0);
    while (c != U_SENTINEL) {
        if (c<32 || c>256) {
            c = '.';
        }
        printf("%c", c);

        c = UTEXT_NEXT32(fPattern);
    }
    printf("\n");
    printf("   Min Match Length:  %d\n", fMinMatchLen);
    printf("   Match Start Type:  %s\n", START_OF_MATCH_STR(fStartType));
    if (fStartType == START_STRING) {
        printf("    Initial match string: \"");
        for (i=fInitialStringIdx; i<fInitialStringIdx+fInitialStringLen; i++) {
            printf("%c", fLiteralText[i]);   // TODO:  non-printables, surrogates.
        }
        printf("\"\n");

    } else if (fStartType == START_SET) {
        int32_t numSetChars = fInitialChars->size();
        if (numSetChars > 20) {
            numSetChars = 20;
        }
        printf("     Match First Chars : ");
        for (i=0; i<numSetChars; i++) {
            UChar32 c = fInitialChars->charAt(i);
            if (0x20<c && c <0x7e) {
                printf("%c ", c);
            } else {
                printf("%#x ", c);
            }
        }
        if (numSetChars < fInitialChars->size()) {
            printf(" ...");
        }
        printf("\n");

    } else if (fStartType == START_CHAR) {
        printf("    First char of Match : ");
        if (0x20 < fInitialChar && fInitialChar<0x7e) {
                printf("%c\n", fInitialChar);
            } else {
                printf("%#x\n", fInitialChar);
            }
    }

    printf("Named Capture Groups:\n");
    if (uhash_count(fNamedCaptureMap) == 0) {
        printf("   None\n");
    } else {
        int32_t pos = UHASH_FIRST;
        const UHashElement *el = NULL;
        while ((el = uhash_nextElement(fNamedCaptureMap, &pos))) {
            const UnicodeString *name = (const UnicodeString *)el->key.pointer;
            char s[100];
            name->extract(0, 99, s, sizeof(s), US_INV);  // capture group names are invariant.
            int32_t number = el->value.integer;
            printf("   %d\t%s\n", number, s);
        }
    }

    printf("\nIndex   Binary     Type             Operand\n" \
           "-------------------------------------------\n");
    for (index = 0; index<fCompiledPat->size(); index++) {
        dumpOp(index);
    }
    printf("\n\n");
#endif
}



UOBJECT_DEFINE_RTTI_IMPLEMENTATION(RegexPattern)

U_NAMESPACE_END
#endif  // !UCONFIG_NO_REGULAR_EXPRESSIONS
