/*
 * UartCoder.h
 *
 *  Created on: Nov 14, 2016
 *      Author: roamer
 */

#ifndef COMMON_UARTCODER_H_
#define COMMON_UARTCODER_H_

#include "coder.h"

class UartCoder : public Coder{
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
