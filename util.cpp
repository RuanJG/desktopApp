/*
 * util.cpp
 *
 *  Created on: Nov 12, 2016
 *      Author: roamer
 */

#include <stdarg.h>
#include <cstdio>
#include "util.h"

std::string Util::bytes2Hex(const uint8_t *data, uint32_t size) {
    static const char* const lut = "0123456789ABCDEF";
    std::string output;
    output.reserve(2 * size);

    for (uint32_t i = 0; i < size; ++i) {
        const uint8_t c = data[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

uint32_t Util::hex2Bytes(const std::string& input, uint8_t *buffer) {
    static const char* const lut = "0123456789ABCDEF";
    static const char* const lut2 = "0123456789abcdef";

    std::string::size_type len = input.length();
    len = len / 2 * 2;

    uint32_t out_size = 0;
    for (std::string::size_type i = 0; i < len; i += 2) {
        int8_t a = input[i];
        const char* p = std::lower_bound(lut, lut + 16, a);
        if (*p != a) p = std::lower_bound(lut2, lut2 + 16, a);
        if (*p != a) return out_size;

        int8_t b = input[i + 1];
        const char* q = std::lower_bound(lut, lut + 16, b);
        if (*q != b) q = std::lower_bound(lut2, lut2 + 16, b);
        if (*q != b) return out_size;

        buffer[out_size++] = (((p - lut) << 4) | (q - lut));
    }

    return out_size;
}

uint8_t Util::crc8(const uint8_t *data, uint32_t size) {
    uint32_t i = 0;
    uint32_t j = 0;
    uint8_t crc = 0;

    for (i = 0; i < size; ++i) {
        crc = crc ^ data[i];
		for (j = 0; j < 8; ++j) {
            if ((crc & 0x01) != 0) {
                crc = (crc >> 1) ^ 0x8C;
		    } else {
                crc >>= 1;
			}
		}
    }
    return crc;
}

std::string Util::format(const char *fmt, ...)
{
	int32_t size = 0;
	char *buffer = NULL;
	va_list ap;

	/* Determine required size */
	va_start(ap, fmt);
	size = vsnprintf(buffer, size, fmt, ap);
	va_end(ap);

	if (size < 0) {
		return std::string();
	}

	size++;             /* For '\0' */
	buffer = new char[size];
	if (NULL == buffer) {
		return std::string();
	}

	va_start(ap, fmt);
	size = vsnprintf(buffer, size, fmt, ap);
	va_end(ap);

	if (size < 0) {
	   delete[] buffer;
	   return std::string();
	}

	std::string str = buffer;
	delete[] buffer;
	return str;
}
