#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "decimal.h"


decimal_Class decimal__getClassFromSignAndKind(decimal__Sign sign, 
    decimal__Kind kind)
{
    if (kind == decimal__KindNormal) {
        return sign == decimal__SignMinus
            ? decimal_NegativeNormal : decimal_PositiveNormal;
    } else if (kind == decimal__KindSubnormal) {
        return sign == decimal__SignMinus
            ? decimal_NegativeSubnormal : decimal_PositiveSubnormal;
    } else if (kind == decimal__KindZero) {
        return sign == decimal__SignMinus
            ? decimal_NegativeZero : decimal_PositiveZero;
    } else if (kind == decimal__KindInfinity) {
        return sign == decimal__SignMinus
            ? decimal_NegativeInfinity : decimal_PositiveInfinity;
    } else if (kind == decimal__KindQNaN) {
        return decimal_QNaN;
    } else if (kind == decimal__KindSNaN) {
        return decimal_SNaN;
    }
}

decimal_Class decimal_getClass(const decimal *x)
{
    return decimal__getClassFromSignAndKind(x->sign, x->kind);
}

/*
 * utility functions.
 */
decimal *decimal_mallocDecimal(int prec)
{
    return (decimal *)malloc(decimal_calcSize(prec));
}

decimal *decimal_reallocDecimal(decimal *x, int prec)
{
    return (decimal *)realloc(x, decimal_calcSize(prec));
}

void decimal_freeDecimal(decimal *x)
{
    free(x);
}

void decimal_setPrecAndEmax(decimal *x, int prec, int emax)
{
    x->prec = prec;
    x->emax = emax;
}

int decimal__getDigitInDeclet(int declet, int pos)
{
    switch (pos) {
    case 0:
        return decimal__getLoDigitInDeclet(declet);
    case 1:
        return decimal__getMidDigitInDeclet(declet);
    case 2:
        return decimal__getHiDigitInDeclet(declet);
    }
}

int decimal__getDigit(decimal *x, int pos)
{
    int declet = x->unit[pos / decimal_DigitCountPerUnit];
    return decimal__getDigitInDeclet(declet, pos % decimal_DigitCountPerUnit);
}

void decimal__setDigit(decimal *x, int pos, int digit)
{
    int unitPos = pos / decimal_DigitCountPerUnit;
    int digitPos = pos % decimal_DigitCountPerUnit;
    int oldDigit = decimal__getDigitInDeclet(x->unit[unitPos], digitPos);
    switch (digitPos) {
    case 0:
        x->unit[unitPos] += digit - oldDigit;
        break;
    case 1:
        x->unit[unitPos] += (digit - oldDigit) * 10;
        break;
    case 2:
        x->unit[unitPos] += (digit - oldDigit) * 100;
        break;
    }
}

int decimal__getTrailingZeroCount(decimal *x)
{
    int count = 0;
    int i;
    int digit;

    for (i = 0; i < decimal_getDigitCount(x); ++i) {
        digit = decimal__getDigit(x, i);
        if (digit) {
            break;
        } else {
            ++count;
        }
    }
    return count;
}

#if 0
int decimal_isZero(decimal *x)
{
    return decimal__getTrailingZeroCount(x) == decimal_getDigitCount(x);
}
#endif

bool decimal__isDigitChar(char ch)
{
    const char *p;

    for (p = decimal__Digits; *p; ++p) {
        if (*p == ch) {
            return true;
        }
    }
    return false;
}

bool decimal__isDigitString(const char *str)
{
    for (; *str; ++str) {
        if (!decimal__isDigitChar(*str)) {
            return false;
        }
    }
    return true;
}

char *decimal_padZero(char *buf, int count)
{
    for (; count > 0; --count) {
        *buf++ = '0';
    }
    return buf;
}

char *decimal__sprintInt(char *buf, int value)
{
    char *p;
    int count;
    int i;
    int rest;

    p = buf;
    if (value < 0) {
        *p++ = '-';
    }

    count = decimal_countDigitOfInt(value);
    rest = value < 0 ? -value : value;
    for (i = 0; i < count; ++i) {
        p[count - 1 - i] = decimal__Digits[rest % 10];
        rest /= 10;
    }
    return p + count;
}

uint32_t decimal_countDigitOfInt(int32_t value)
{
    uint32_t count;

    if (value == 0) {
        return 1;
    }

    count = 0;
    for (; value; value /= 10) {
        ++count;
    }
    return count;
}

#if 0

static char *decimal_sprintDigits(char *buf, uint8_t *digits, uint32_t offset,
    uint32_t count);
static char *decimal_strncpy(char *buf, const char *source, uint32_t count);
static void decimal_clearSignificand(decimal *number);
static bool decimal_strncasematch(const char *target, const char *str_upper,
    const char *str_lower, uint32_t count);
static void decimal_setDigitsWithChars(decimal *number, const char *chars,
    uint32_t count);
static void decimal_setDigitsWithStr(decimal *number, const char *str);

static void decimal_zeroDigits(decimal *number, uint32_t offset,
    uint32_t count);
static bool decimal_hasAllZeroDigits(const decimal *number, uint32_t offset,
    uint32_t count);

static void decimal_setLargestFiniteMagnitude(decimal *number,
    DecimalContext *context);

static void decimal_clearSignificand(decimal *number)
{
    uint32_t unit_count;
    uint32_t i;

    unit_count = decimal_getUnitCount(decimal_getCapacity(number));
    for (i = 0; i < unit_count; ++i) {
        number->significand[i] = 0;
    }
}

void decimal_init(decimal *number, uint32_t capacity)
{
    uint32_t unit_count;
    uint32_t i;

    number->flags = 0;
    decimal_resetMinusSign(number);
    decimal_setKind(number, decimal_KindFinite);
    decimal_setExponent(number, 0);
    decimal_setCapacity(number, capacity);
    decimal_setLength(number, 1);
    decimal_clearSignificand(number);
}

bool decimal_isNormal(const decimal *number)
{
    return decimal_isFinite(number)
        && decimal_getSignificandDigit(number, 0) != 0;
}

bool decimal_isSubnormal(const decimal *number)
{
    return decimal_isFinite(number)
        && decimal_getSignificandDigit(number, 0) == 0
        && !decimal_isZero(number);
}

bool decimal_isZero(const decimal *number)
{
    uint32_t i;

    for (i = 0; i < decimal_getLength(number); ++i) {
        if (decimal_getSignificandDigit(number, i) != 0) {
            return false;
        }
    }
    return true;
}

void decimal_setZero(decimal *number)
{
    decimal_resetMinusSign(number);
    decimal_setKind(number, decimal_KindFinite);
    decimal_resetSignaling(number);
    decimal_setExponent(number, 0);
    decimal_setLength(number, 1);
    decimal_clearSignificand(number);
}

void decimal_setInfinite(decimal *number)
{
    decimal_setKind(number, decimal_KindInfinite);
    decimal_resetSignaling(number);
    decimal_setExponent(number, 0);
    decimal_setLength(number, 1);
    decimal_clearSignificand(number);
}

void decimal_setSNaN(decimal *number)
{
    decimal_resetMinusSign(number);
    decimal_setKind(number, decimal_KindNaN);
    decimal_setSignaling(number);
    decimal_setExponent(number, 0);
    decimal_setLength(number, 1);
    decimal_clearSignificand(number);
}

void decimal_setQNaN(decimal *number)
{
    decimal_resetMinusSign(number);
    decimal_setKind(number, decimal_KindNaN);
    decimal_resetSignaling(number);
    decimal_setExponent(number, 0);
    decimal_setLength(number, 1);
    decimal_clearSignificand(number);
}

void decimal_copy(decimal *result, const decimal *number)
{
    if (result != number) {
        *result = *number;
    }
}

void decimal_negate(decimal *result, const decimal *number)
{
    decimal_copy(result, number);
    result->flags ^= decimal_MinusSignBit;
}

void decimal_abs(decimal *result, const decimal *number)
{
    decimal_copy(result, number);
    decimal_resetMinusSign(result);
}

void decimal_copySign(decimal *result, const decimal *number)
{
    decimal_copy(result, number);
    if (decimal_isSignMinus(number)) {
        decimal_setMinusSign(result);
    } else {
        decimal_resetMinusSign(result);
    }
}

bool decimal_convertFromDecimalCharacter(decimal *number, const char *str)
{
    const char *p;
    const char *q;
    const char *start;
    bool negative;
    uint32_t length;
    uint32_t digit_pos;
    int32_t exponent_offset;
    int32_t exponent;
    bool exponent_negative;
    const char *dot;
    bool seen_dot;

    p = str;
    negative = false;
    if (*p == '+') {
        ++p;
    } else if (*p == '-') {
        negative = true;
        ++p;
    }
    if (negative) {
        decimal_setMinusSign(number);
    } else {
        decimal_resetMinusSign(number);
    }

    decimal_clearSignificand(number);

    if (decimal__isDigitChar(*p) || *p == '.') {
        start = p;
        dot = NULL;

        digit_pos = 0;
        exponent_offset = 0;
        while (*p == '0') {
            ++p;
        }

        if (*p == '.') {
            dot = p++;

            while (*p == '0') {
                ++p;
            }
// 0.1 -> 1E-1
// 0.001 -> 1E-3
// 0. -> 0E0
// .0 -> 0.0E0
// . -> error
            if (decimal__isDigitChar(*p)) {
                exponent_offset = -(p - dot);
                while (decimal__isDigitChar(*p)) {
                    decimal_setSignificandDigit(number, digit_pos++,
                        *p++ - '0');
                }
            } else {
                if (p == start + 1) { // single period and no digit
                    return false;
                }

                decimal_setSignificandDigit(number, digit_pos++, 0);
                for (q = dot + 1; q < p; ++q) {
                    decimal_setSignificandDigit(number, digit_pos++, 0);
                }
            }
        } else {
            while (decimal__isDigitChar(*p)) {
                decimal_setSignificandDigit(number, digit_pos++, *p++ - '0');
            }
            if (digit_pos > 0) {
                exponent_offset = digit_pos - 1;
            }
            if (*p == '.') {
                ++p;
                while (decimal__isDigitChar(*p)) {
                    decimal_setSignificandDigit(number, digit_pos++,
                        *p++ - '0');
                }
            }
        }

        decimal_setLength(number, digit_pos);

        exponent = 0;
        if (*p == 'E' || *p == 'e') {
            ++p;
            exponent_negative = false;
            if (*p == '+') {
                ++p;
            } else if (*p == '-') {
                exponent_negative = true;
                ++p;
            }

            if (decimal__isDigitString(p)) {
                exponent = atoi(p); // FIXME check range
                if (exponent_negative) {
                    exponent = -exponent;
                }
            } else {
                return false;
            }
        }

// 12.3E+0 -> 1.23E+1
        exponent += exponent_offset;
        decimal_setExponent(number, exponent);
        decimal_setKind(number, decimal_KindFinite);
    } else if (decimal_strncasematch(p, "INFINITY", "infinity",
            sizeof("infinity") - 1)
    ) {
        if (p[sizeof("infinity") - 1] == '\0') {
            decimal_setKind(number, decimal_KindInfinite);
            decimal_setLength(number, 0);
        } else {
            return false;
        }
    } else if (decimal_strncasematch(p, "INF", "inf", sizeof("inf") - 1)) {
        if (p[sizeof("inf") - 1] == '\0') {
            decimal_setKind(number, decimal_KindInfinite);
            decimal_setLength(number, 0);
        } else {
            return false;
        }
    } else if (decimal_strncasematch(p, "NAN", "nan", sizeof("nan") - 1)) {
        p += sizeof("nan") - 1;
        if (*p == '\0') {
            decimal_setKind(number, decimal_KindNaN);
            decimal_resetSignaling(number);
            decimal_setLength(number, 0);
        } else if (decimal__isDigitString(p)) {
            decimal_setKind(number, decimal_KindNaN);
            decimal_resetSignaling(number);
            decimal_setDigitsWithStr(number, p);
        } else {
            // FIXME set error
        }
    } else if (decimal_strncasematch(p, "SNAN", "snan", sizeof("snan") - 1)) {
        p += sizeof("snan") - 1;
        if (*p == '\0') {
            decimal_setKind(number, decimal_KindNaN);
            decimal_setSignaling(number);
            decimal_setLength(number, 0);
        } else if (decimal__isDigitString(p)) {
            decimal_setKind(number, decimal_KindNaN);
            decimal_setSignaling(number);
            decimal_setDigitsWithStr(number, p);
        } else {
            return false;
        }
    } else {
        return false;
    } 
    return true;
}

char *decimal_convertToDecimalCharacter(decimal *number)
{
}

void decimal_convertToDecimalNonExponential(char *result, decimal *number)
{
    char *p;
    uint32_t kind;
    uint32_t length;
    int32_t exponent;
    int32_t pos;

    p = result;
    if (decimal_isSignMinus(number)) {
        *p++ = '-';
    }

    kind = decimal_getKind(number);
    switch (kind) {
    case decimal_KindFinite:
        length = decimal_getLength(number);
        exponent = decimal_getExponent(number);
        pos = 1 + exponent;
// 1.230E+4 = 12300
// 1.230E+3 = 1230
// 1.230E+2 = 123.0
// 1.230E+1 = 12.30
// 1.230E+0 = 1.230
// 1.230E-1 = 0.1230
// 1.230E-2 = 0.01230
// 1.230E-3 = 0.001230
// 0.1230E+0 = 0.1230
// 0.01230E+0 = 0.01230
// 0.01230E-1 = 0.001230
        if (pos > 0) {
            if (pos < length) {
                p = decimal_sprintDigits(p, number->significand, 0, pos);
                *p++ = '.';
                p = decimal_sprintDigits(p, number->significand, pos,
                    length - pos);
            } else if (pos > length) {
                p = decimal_sprintDigits(p, number->significand, 0, length);
                p = decimal_padZero(p, pos - length);
            } else {
                p = decimal_sprintDigits(p, number->significand, 0, length);
            }
        } else {
            *p++ = '0';
            *p++ = '.';
            if (pos < 0) {
                p = decimal_padZero(p, -pos);
            }
            p = decimal_sprintDigits(p, number->significand, 0, length);
        }
        break;
    case decimal_KindInfinite:
        p = decimal_strncpy(p, decimal_InfinityLiteral,
            sizeof(decimal_InfinityLiteral) - 1);
        break;
    case decimal_KindNaN:
        if (decimal_isSignaling(number)) {
            p = decimal_strncpy(p, decimal_sNaNLiteral,
                sizeof(decimal_sNaNLiteral) - 1);
        } else {
            p = decimal_strncpy(p, decimal_NaNLiteral,
                sizeof(decimal_NaNLiteral) - 1);
        }
        length = decimal_getLength(number);
        if (length > 0) {
            p = decimal_sprintDigits(p, number->significand, 0, length);
        }
        break;
    }
    *p = '\0';
}

uint32_t decimal_getLengthOfDecimalNonExponential(decimal *number)
{
    uint32_t kind;
    uint32_t length;
    uint32_t digit_count;
    int32_t exponent;
    int32_t pos;

    length = decimal_isSignMinus(number) ? sizeof("-") - 1 : 0;
    kind = decimal_getKind(number);
    switch (kind) {
    case decimal_KindFinite:
        digit_count = decimal_getLength(number);
        length += digit_count;
        exponent = decimal_getExponent(number);
        pos = 1 + exponent;
// 1.230E+4 = 12300
// 1.230E+3 = 1230
// 1.230E+2 = 123.0
// 1.230E+1 = 12.30
// 1.230E+0 = 1.230
// 1.230E-1 = 0.1230
// 1.230E-2 = 0.01230
// 0.1230E+0 = 0.1230
// 0.01230E+0 = 0.01230
// 0.01230E-1 = 0.001230
        if (pos > 0) {
            if (pos < digit_count) {
                length += sizeof(".") - 1;
            } else if (pos > length) {
                length += pos - length;
            }
        } else {
            length += sizeof("0.") - 1 - pos;
        }
        break;
    case decimal_KindInfinite:
        length += sizeof(decimal_InfinityLiteral) - 1;
        break;
    case decimal_KindNaN:
        if (decimal_isSignaling(number)) {
            length += (sizeof(decimal_sNaNLiteral) - 1)
                + decimal_getLength(number);
        } else {
            length += (sizeof(decimal_NaNLiteral) - 1)
                + decimal_getLength(number);
        }
        break;
    }
    return length;
}

void decimal_convertToDecimalExponential(char *result, decimal *number)
{
    char *p;
    uint32_t kind;
    uint32_t length;
    int32_t exponent;

    p = result;
    if (decimal_isSignMinus(number)) {
        *p++ = '-';
    }

    kind = decimal_getKind(number);
    switch (kind) {
    case decimal_KindFinite:
        *p++ = decimal__Digits[decimal_getSignificandDigit(number, 0)];
        length = decimal_getLength(number);
        if (length > 1) {
            *p++ = '.';
            p = decimal_sprintDigits(p, number->significand, 1, length - 1);
        }
        *p++ = 'E';
        exponent = decimal_getExponent(number);
        if (exponent >= 0) {
            *p++ = '+';
        }
        p = decimal__sprintInt(p, exponent);
        break;
    case decimal_KindInfinite:
        p = decimal_strncpy(p, decimal_InfinityLiteral,
            sizeof(decimal_InfinityLiteral) - 1);
        break;
    case decimal_KindNaN:
        if (decimal_isSignaling(number)) {
            p = decimal_strncpy(p, decimal_sNaNLiteral,
                sizeof(decimal_sNaNLiteral) - 1);
        } else {
            p = decimal_strncpy(p, decimal_NaNLiteral,
                sizeof(decimal_NaNLiteral) - 1);
        }
        length = decimal_getLength(number);
        if (length > 0) {
            p = decimal_sprintDigits(p, number->significand, 0, length);
        }
        break;
    }
    *p = '\0';
}

uint32_t decimal_getLengthOfDecimalExponential(decimal *number)
{
    uint32_t kind;
    uint32_t length;
    uint32_t digit_count;

    length = decimal_isSignMinus(number) ? sizeof("-") - 1 : 0;
    kind = decimal_getKind(number);
    switch (kind) {
    case decimal_KindFinite:
        digit_count = decimal_getLength(number);
        if (digit_count > 1) {
            length += sizeof(".") - 1;
        }
        length += digit_count + (sizeof("E") - 1)
            + (sizeof("+") - 1)
            + decimal_countDigitOfInt(decimal_getExponent(number));
        break;
    case decimal_KindInfinite:
        length += sizeof(decimal_InfinityLiteral) - 1;
        break;
    case decimal_KindNaN:
        if (decimal_isSignaling(number)) {
            length += (sizeof(decimal_sNaNLiteral) - 1)
                + decimal_getLength(number);
        } else {
            length += (sizeof(decimal_NaNLiteral) - 1)
                + decimal_getLength(number);
        }
        break;
    }
    return length;
}

static char *decimal_sprintDigits(char *buf, uint8_t *digits, uint32_t offset,
    uint32_t count)
{
    char *p;
    uint32_t i;

    p = buf;
    for (i = 0; i < count; ++i) {
        *p++ = decimal__Digits[decimal_getDigit(digits, i + offset)];
    }
    return p;
}

static char *decimal_strncpy(char *buf, const char *source, uint32_t count)
{
    uint32_t i;

    for (i = 0; i < count; ++i) {
        *buf++ = *source++;
    }
    return buf;
}

static bool decimal_strncasematch(const char *target, const char *str_upper,
    const char *str_lower, uint32_t count)
{
    uint32_t i;

    for (i = 0; i < count; ++i) {
        if (!(target[i] == str_upper[i] || target[i] == str_lower[i])) {
            return false;
        }
    }
    return true;
}

static void decimal_setDigitsWithChars(decimal *number, const char *chars,
    uint32_t count)
{
    uint32_t i;

    for (i = 0; i < count; ++i, ++chars) {
        decimal_setSignificandDigit(number, i, *chars - '0');
    }
}

static void decimal_setDigitsWithStr(decimal *number, const char *str)
{
    uint32_t length;

    length = strlen(str);
    decimal_setDigitsWithChars(number, str, length);
    decimal_setLength(number, length);
}

void decimal_setDigit(uint8_t *digits, uint32_t pos, uint8_t digit)
{
    uint32_t offset;

    offset = pos / 2;
    digits[offset] = (pos % 2
        ? (digits[offset] & 0xf0) | digit
        : (digit << 4) | (digits[offset] & 0x0f));
}

uint8_t decimal_getDigit(const uint8_t *digits, uint32_t pos)
{
    return pos % 2 ? digits[pos / 2] & 0x0f : digits[pos / 2] >> 4;
}

static void decimal_incrementMagnitude(decimal *number, DecimalContext *context)
{
    int32_t i;
    uint8_t digit;
    uint32_t length;

    for (i = decimal_getLength(number) - 1; i >= 0; --i) {
        digit = decimal_getSignificandDigit(number, i) + 1;
        if (digit < decimal__Radix) {
            decimal_setSignificandDigit(number, i, digit);
            return;
        } else {
            decimal_setSignificandDigit(number, i, 0);
        }
    }

    decimal_setExponent(number, decimal_getExponent(number) + 1);
    if (decimal_getExponent(number) > DecimalContext_getMaxExponent(context)) {
        decimal_setInfinite(number);
        DecimalContext_raiseFlags(context, decimal_Overflow);
    } else {
        if (decimal_getLength(number) < DecimalContext_getPrecision(context) &&
            DecimalContext_getPrecision(context) < decimal_getCapacity(number)
        ) {
            decimal_setLength(number, decimal_getLength(number) + 1);
        }
        for (i = decimal_getLength(number) - 1; i > 0; --i) {
            decimal_setSignificandDigit(number, i,
                decimal_getSignificandDigit(number, i - 1));
        }
        decimal_setSignificandDigit(number, 0, 1);
    }
}

static void decimal_zeroDigits(decimal *number, uint32_t offset,
    uint32_t count)
{
    uint32_t i;

    for (i = 0; i < count; ++i) {
        decimal_setSignificandDigit(number, i + offset, 0);
    }
}

static bool decimal_hasAllZeroDigits(const decimal *number, uint32_t offset,
    uint32_t count)
{
    uint32_t i;

    for (i = 0; i < count; ++i) {
        if (decimal_getSignificandDigit(number, i + offset) != 0) {
            return false;
        }
    }
    return true;
}

void decimal_round(decimal *result, const decimal *number,
    DecimalContext *context)
{
    uint32_t prec;
    uint32_t i;
    uint32_t flags;
    uint8_t digit;
    bool wasAllZero;
    bool restWasAllZero;
    int32_t emax;

    decimal_copy(result, number);
    prec = DecimalContext_getPrecision(context);
    emax = DecimalContext_getMaxExponent(context);
    flags = 0;
    switch (DecimalContext_getRounding(context)) {
    case decimal_RoundTiesToEven:
        if (decimal_getLength(number) > prec) {
            digit = decimal_getSignificandDigit(number, prec);
            restWasAllZero = decimal_hasAllZeroDigits(number, prec + 1,
                decimal_getLength(number) - (prec + 1));
            if (!(digit == 0 && restWasAllZero)) {
                flags |= decimal_Inexact;
                decimal_zeroDigits(result, prec,
                    decimal_getLength(result) - prec);
            }
            decimal_setLength(result, prec);
            if (digit > 5
                || (digit == 5
                    && (!restWasAllZero
                        || (decimal_getSignificandDigit(number, prec - 1) % 2)))
            ) {
                decimal_incrementMagnitude(result, context);
            }
        }

        if (decimal_getExponent(result) > emax) {
            decimal_setInfinite(result);
            flags |= decimal_Inexact | decimal_Overflow;
        }
        break;
    case decimal_RoundTiesToAway:
        if (decimal_getLength(number) > prec) {
            digit = decimal_getSignificandDigit(number, prec);
            wasAllZero = decimal_hasAllZeroDigits(number, prec,
                decimal_getLength(number) - prec);
            if (!wasAllZero) {
                flags |= decimal_Inexact;
                decimal_zeroDigits(result, prec,
                    decimal_getLength(result) - prec);
            }
            decimal_setLength(result, prec);
            if (!wasAllZero && digit >= 5) {
                decimal_incrementMagnitude(result, context);
            }
        }

        if (decimal_getExponent(result) > emax) {
            decimal_setInfinite(result);
            flags |= decimal_Inexact | decimal_Overflow;
        }
        break;
    case decimal_RoundTowardPositive:
        wasAllZero = true;
        if (decimal_getLength(number) > prec) {
            wasAllZero = decimal_hasAllZeroDigits(number, prec,
                decimal_getLength(number) - prec);
            if (!wasAllZero) {
                flags |= decimal_Inexact;
                decimal_zeroDigits(result, prec,
                    decimal_getLength(result) - prec);
            }
            decimal_setLength(result, prec);
        }
        if (!wasAllZero && !decimal_isSignMinus(number)) {
            decimal_incrementMagnitude(result, context);

            if (decimal_getExponent(result) > emax) {
                decimal_setInfinite(result);
                flags |= decimal_Inexact | decimal_Overflow;
            }
        } else {
            if (decimal_getExponent(result) > emax) {
                decimal_setLargestFiniteMagnitude(result, context);
                flags |= decimal_Inexact | decimal_Overflow;
            }
        }
        break;
    case decimal_RoundTowardNegative:
        if (decimal_getLength(number) > prec) {
            wasAllZero = decimal_hasAllZeroDigits(number, prec,
                decimal_getLength(number) - prec);
            if (!wasAllZero) {
                flags |= decimal_Inexact;
                decimal_zeroDigits(result, prec,
                    decimal_getLength(result) - prec);
            }
            decimal_setLength(result, prec);
        }
        if (flags && decimal_isSignMinus(number)) {
            decimal_incrementMagnitude(result, context);

            if (decimal_getExponent(result) > emax) {
                decimal_setInfinite(result);
                flags |= decimal_Inexact | decimal_Overflow;
            }
        } else {
            if (decimal_getExponent(result) > emax) {
                decimal_setLargestFiniteMagnitude(result, context);
                flags |= decimal_Inexact | decimal_Overflow;
            }
        }
        break;
    case decimal_RoundTowardZero:
        wasAllZero = true;
        if (decimal_getLength(number) > prec) {
            wasAllZero = decimal_hasAllZeroDigits(number, prec,
                decimal_getLength(number) - prec);
            if (!wasAllZero) {
                flags |= decimal_Inexact;
                decimal_zeroDigits(result, prec,
                    decimal_getLength(result) - prec);
            }
            decimal_setLength(result, prec);
        }

        if (decimal_getExponent(result) > emax) {
            decimal_setLargestFiniteMagnitude(result, context);
            flags |= decimal_Inexact | decimal_Overflow;
        }
        break;
    default:
        flags |= decimal_InvalidOperation;
        break;
    }
    DecimalContext_raiseFlags(context, flags);
}

void decimal_debugPrintDecimalExponential(decimal *number)
{
    char buf[512];
    uint32_t length;
    char *p;

    length = decimal_getLengthOfDecimalNonExponential(number) + 1;
    if (length <= sizeof(buf)) {
        p = buf;
    } else {
        p = (char *)malloc(sizeof(char) * length);
    }
    decimal_convertToDecimalExponential(p, number);
    printf("%s\n", p);
    if (p != buf) {
        free(p);
    }
}

static void decimal_setLargestFiniteMagnitude(decimal *number,
    DecimalContext *context)
{
    uint32_t i;
    uint32_t precision;

    decimal_setKind(number, decimal_KindFinite);
    decimal_setExponent(number, DecimalContext_getMaxExponent(context));
    precision = DecimalContext_getPrecision(context);
    decimal_setLength(number, precision);
    for (i = 0; i < precision; ++i) {
        decimal_setSignificandDigit(number, i, 9);
    }
    decimal_zeroDigits(number, precision,
        decimal_getCapacity(number) - precision);
}
#endif

/*
 
1.23E2 + 2.54E3
1.23E2 + 25.4E2

 9
 9
12

*/
