/*
 * util.h
 *
 *  Created on: Nov 12, 2016
 *      Author: roamer
 */

#ifndef COMMON_UTIL_H_
#define COMMON_UTIL_H_

#include <stdint.h>
#include <string>
#include <list>

class Util {
public:
	static std::string bytes2Hex(const uint8_t *data, uint32_t size);
	static uint32_t hex2Bytes(const std::string& input, uint8_t *buffer);
	static uint8_t crc8(const uint8_t *data, uint32_t size);
	static std::string format(const char *fmt, ...);
};

#endif /* COMMON_UTIL_H_ */
