/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "MString.h"

#include <cmath>
#include <cstdlib>

using namespace mc6809;

static constexpr uint32_t MaxFloatDigits = 16;

mc6809::String& mc6809::String::erase(uint16_t pos, uint16_t len)
{
    if (pos >= _size - 1) {
        return *this;
    }
    if (pos + len >= _size) {
        len = _size - pos - 1;
    }
    memmove(_data + pos, _data + pos + len, _size - pos - len);
    _size -= len;
    return *this;
}

mc6809::String mc6809::String::slice(int32_t start, int32_t end) const
{
    int32_t sz = static_cast<int32_t>(size());
    if (start < 0) {
        start = sz + start;
    }
    if (end < 0) {
        end = sz + end;
    }
    if (end > sz) {
        end = sz;
    }
    if (start >= end) {
        return String();
    }
    return String(_data + start, end - start);
}

mc6809::String mc6809::String::trim() const
{
    if (_size < 2 || !_data) {
        return String();
    }
    uint16_t l = _size - 1;
    char* s = _data;
    while (isspace(s[l - 1])) {
        --l;
    }
    while (*s && isspace(*s)) {
        ++s;
        --l;
    }
    return String(s, static_cast<int32_t>(l));
}

Vector<mc6809::String> mc6809::String::split(const mc6809::String& separator, bool skipEmpty) const
{
    Vector<String> array;
    if (size() == 0) {
        return array;
    }
    char* p = _data;
    assert(p);
    while (1) {
        char* n = strstr(p, separator.c_str());
        if (!n || n - p != 0 || !skipEmpty) {
            array.push_back(String(p, static_cast<int32_t>(n ? (n - p) : -1)));
        }
        if (!n) {
            break;
        }
        p = n ? (n + separator.size()) : nullptr;
    }
    return array;
}

mc6809::String mc6809::String::join(const Vector<mc6809::String>& array, const mc6809::String& separator)
{
    String s;
    bool first = true;
    for (auto it : array) {
        if (first) {
            first = false;
        } else {
            s += separator;
        }
        s += it;
    }
    return s;
}

mc6809::String mc6809::String::join(const Vector<char>& array)
{
    String s;
    s.ensureCapacity(array.size());
    for (auto it : array) {
        s += it;
    }
    return s;
}
void mc6809::String::doEnsureCapacity(uint16_t size)
{
    _capacity = _capacity ? _capacity * 2 : 1;
    if (_capacity < size) {
        _capacity = size;
    }
    char* newData = new char[_capacity];
    if (_data) {
        if (newData) {
            memcpy(newData, _data, _size);
        }
        delete [ ] _data;
    }
    
    if (!newData) {
        _capacity = 0;
        _size = 1;
    }
    _data = newData;
}

static int32_t intToString(int64_t x, char* str, int16_t dp, uint8_t decimalDigits)
{
    // Adjust x and dp for decimalDigits
    if (dp > decimalDigits) {
        int16_t exp = dp - decimalDigits;
        while (exp-- > 1) {
            x /= 10;
        }
        
        // We've tossed all but one digit. Round and then toss it
        x = (x + 5) / 10;
        dp = decimalDigits;
    }
    
    int32_t i = 0;
    bool haveDP = false;
    
    while (x) {
        str[i++] = (x % 10) + '0';
        x /= 10;
        if (--dp == 0) {
            str[i++] = '.';
            haveDP = true;
        }
    }
    
    if (dp > 0) {
        while (dp--) {
            str[i++] = '0';
        }
        str[i++] = '.';
        haveDP = true;
    }
    assert(i > 0);
    if (str[i-1] == '.') {
        str[i++] = '0';
    }
    
    std::reverse(str, str + i);
    str[i] = '\0';

    if (haveDP) {
        i--;
        while (str[i] == '0') {
            str[i--] = '\0';
        }
        if (str[i] == '.') {
            str[i--] = '\0';
        }
        i++;
    }

    return i;
}

static bool toString(char* buf, int64_t value, int16_t& exp, uint8_t decimalDigits)
{
    // Value is guaranteed to be non-negative
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        exp = 0;
        return true;
    }
    
    if (!exp) {
        intToString(value, buf, 0, decimalDigits);
        return true;
    }

    // See how many digits we have
    int64_t v = value;
    int digits = 0;
    for ( ; v > 0; ++digits, v /= 10) ;
    v = value;
    int32_t dp;
    if (exp + digits > MaxFloatDigits || -exp > MaxFloatDigits) {
        // Scientific notation
        dp = digits - 1;
        exp += dp;
    } else {
        dp = -exp;
        exp = 0;
        
        if (digits - dp < decimalDigits) {
            // Number is of the form xxx.yyy. Make total digits equal to decimalDigits
            decimalDigits -= (digits - dp);
        }
    }
    
    int32_t i = intToString(value, buf, dp, decimalDigits);
    
    if (exp) {
        buf[i++] = 'e';
        if (exp < 0) {
            buf[i++] = '-';
            exp = -exp;
        }
        intToString(exp, buf + i, 0, decimalDigits);
    }
    
    return true;
}

#include <string>

static void decompose(double f, int64_t& mantissa, int16_t& exp)
{
    // Make the number fit 16 digits
    static constexpr double max = 1e16;
    static constexpr double min = 1e15;
    
    exp = 0;
    while (f < min) {
        f *= 10;
        --exp;
    }
    while (f > max) {
        f /= 10;
        ++exp;
    }
    mantissa = int64_t(f);
}

mc6809::String::String(double value, uint8_t decimalDigits)
{
    //          sign    digits  dp      'e'     dp      exp     '\0'
    char buf[   1 +     16 +    1 +     1 +     1 +     3 +     1];
    
    int64_t mantissa;
    int16_t exp;
    decompose(value, mantissa, exp);
    
    if (mantissa < 0) {
        buf[0] = '-';
        ::toString(buf + 1, -mantissa, exp, decimalDigits);
    } else {
        ::toString(buf, mantissa, exp, decimalDigits);
    }
    *this = String(buf);
}

mc6809::String::String(uint32_t value)
{
    char buf[12];
    int16_t exp = 0;
    ::toString(buf, value, exp, 0);
    *this = String(buf);
}

mc6809::String::String(int32_t value)
{
    String s;
    if (value < 0) {
        *this = String('-') + String(static_cast<uint32_t>(-value));
    } else {
        *this = String(static_cast<uint32_t>(value));
    }
}

mc6809::String::String(void* value)
{
    // Convert to a uint32_t. This will truncate the pointer on Mac
    *this = String::format("0x%08x", static_cast<uint32_t>(reinterpret_cast<intptr_t>(value)));
}

mc6809::String mc6809::String::prettySize(uint32_t size, uint8_t decimalDigits, bool binary)
{
    mc6809::String s;
    int32_t multiplier = binary ? 1024 : 1000;
    
    if (static_cast<int32_t>(size) < multiplier) {
        return String(size) + ' ';
    } else if (static_cast<int32_t>(size) < multiplier * multiplier) {
        return String(float(size) / multiplier, decimalDigits) + " K";
    } else if (static_cast<int32_t>(size) < multiplier * multiplier * multiplier) {
        return String(float(size) / multiplier / multiplier, decimalDigits) + " M";
    } else {
        return String(float(size) / multiplier / multiplier / multiplier, decimalDigits) + " G";
    }
}

mc6809::String String::vformat(const char* fmt, va_list args)
{
    va_list args2;
    va_copy(args2, args);
    size_t size = ::vsnprintf(nullptr, 0, fmt, args) + 1;
    if( size <= 0 ) {
        return "***** Error during formatting.";
    }
    char* buf(new char[size]); 
    ::vsnprintf(buf, size, fmt, args2);
    String s(buf, int32_t(size - 1));
    delete [ ] buf;
    return s;
}

mc6809::String String::format(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    return vformat(fmt, args);
}
