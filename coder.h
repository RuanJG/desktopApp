/*
 * coder.h
 *
 *  Created on: Nov 21, 2016
 *      Author: roamer
 */

#ifndef COMMON_CODER_H_
#define COMMON_CODER_H_

#include <list>
#include "chunk.h"

class Coder {
public:
	virtual ~Coder() { };
    virtual bool encode(const unsigned char *data, int size, Chunk &chunk) = 0;
    virtual void decode(const unsigned char *data, int size, std::list<Chunk> &chunks) = 0;
	virtual void reset() = 0;
};

#endif /* COMMON_CODER_H_ */
