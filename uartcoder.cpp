/*
 * UartCoder.cpp
 *
 *  Created on: Nov 14, 2016
 *      Author: roamer
 */

#include <string.h>

#include "uartcoder.h"
#include "util.h"
#include <stdio.h>

#define PACKET_START         0xAC
#define PACKET_END           0xAD
#define PACKET_ESCAPE        0xAE
#define PACKET_ESCAPE_MASK   0x80
#define MAX_PACKET_SIZE      (1024 * 128)

#define APPEND_BYTE(chunk, byte) \
do { \
	uint8_t ubyte = (uint8_t)(byte); \
    if (PACKET_START == ubyte || PACKET_END == ubyte || PACKET_ESCAPE == ubyte) { \
        chunk.append((unsigned char) PACKET_ESCAPE); \
        chunk.append((unsigned char) (PACKET_ESCAPE_MASK ^ ubyte)); \
	} else { \
        chunk.append((unsigned char) ubyte); \
	} \
} while(0)

UartCoder::UartCoder() :
	m_cache()
{
	;
}

bool UartCoder::encode(const unsigned char *data, int size, Chunk &chunk) {
	chunk.extend((size + 3) * 2);
    chunk.append((unsigned char) PACKET_START);

	for (int index = 0; index < size; ++index) {
		APPEND_BYTE(chunk, data[index]);
	}

	uint8_t crc = Util::crc8((const uint8_t *)data, size);
	APPEND_BYTE(chunk, crc);

    chunk.append((unsigned char) PACKET_END);
	return true;
}

void UartCoder::decode(const unsigned char *data, int size, std::list<Chunk> &chunks) {
	m_cache.append(data, size);

	while (true) {
		data = m_cache.getData();
		size = m_cache.getSize();
		if (size < 1) {
			break;
		}

		if (size > MAX_PACKET_SIZE) {
			m_cache.clear();
			break;
		}

        const unsigned char *start = (const unsigned char *) memchr(data, PACKET_START, size);
		if (NULL == start) {
			m_cache.clear();
			break;
		}

        const unsigned char *end = (const unsigned char *) memchr(data, PACKET_END, size);
		if (NULL == end) {
			break;
		}

		Chunk chunk;
		parsePacket(start, end - start + 1, chunk);
		m_cache.trim(end - data + 1);

		if (chunk.getSize() > 0) {
			chunks.push_back(chunk);
		}
	}
}

void UartCoder::reset() {
	m_cache.clear();
}

void UartCoder::parsePacket(const unsigned char *data, \
		int size, Chunk &chunk) const {
	for (int index = 0; index < size; ++index) {
		uint8_t byte = (uint8_t) data[index];
		if ((PACKET_START == byte) || (PACKET_END == byte)) {
			continue;
		}

		if (PACKET_ESCAPE == byte) {
			++index;

			if (index >= size) {
				chunk.clear();
				return;
			}

			byte = (uint8_t) data[index];
			byte = byte ^ PACKET_ESCAPE_MASK;
		}
        chunk.append((unsigned char) byte);
	}

	data = chunk.getData();
	size = chunk.getSize();
	if (size < 2) {
		chunk.clear();
		return;
	}

	uint8_t crc8 = data[size - 1];
	if (crc8 != Util::crc8((const uint8_t *)data, size - 1)) {
		chunk.clear();
		return;
	}

	chunk.trim(-1); // remove the crc8 byte!
}
