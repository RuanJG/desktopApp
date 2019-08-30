/*
 * UartCoder.h
 *
 *  Created on: Nov 14, 2016
 *      Author: roamer
 */

#include <libs/chunk.h>
#include <list>

#ifndef COMMON_UARTCODER_H_
#define COMMON_UARTCODER_H_


class UartCoder {
private:
	Chunk m_cache;

public:
	UartCoder();
    bool encode(const unsigned char *data, int size, Chunk &chunk);
    void decode(const unsigned char *data, int size, std::list<Chunk> &chunks);
	void reset();

private:
    void parsePacket(const unsigned char *data, int size, Chunk &chunk) const;
};

#endif /* COMMON_UARTCODER_H_ */
