/*
 * chunk.h
 *
 *  Created on: Apr 11, 2016
 *      Author: roamer
 */

#ifndef MSG_CHUNK_H_
#define MSG_CHUNK_H_

class Chunk {
private:
    unsigned char *m_data;
	int   m_size;
	int   m_bufSize;
	int   m_start;

public:
    Chunk(const unsigned char *data, int size);
	Chunk(const Chunk &chunk);
	Chunk();

	Chunk &operator=(const Chunk &chunk);
	virtual ~Chunk();

	int getSize() const;
    const unsigned char *getData() const;

    unsigned char *takeData();
	void trnasferTo(Chunk &chunk);
    void takeFrom(unsigned char *data, int size);
    void copyFrom(const unsigned char *data, int size);

	void trim(int size);

    void append(const unsigned char *data, int size);
    void append(unsigned char data);
	void append(const Chunk &chunk);

	void extend(int max);
	void clear();
};

#endif /* MSG_CHUNK_H_ */
