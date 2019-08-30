/*
 * chunk.cpp
 *
 *  Created on: Apr 11, 2016
 *      Author: roamer
 */

#include <cstring>
#include <cstdlib>
#include "chunk.h"

#define NOT_OVERLAP(m1, s1, m2, s2) \
	((((m1) + (s1)) <= (m2)) || ((m2) + (s2)) <= (m1))

void Chunk::clear() {
	m_start = 0;
	m_size = 0;
}

void Chunk::copyFrom(const unsigned char *data, int size) {
	this->clear();
	if (size < 1) {
		return;
	}

	if (m_bufSize >= size) {
		m_start = 0;
		m_size = size;
		memmove(m_data, data, size);
		return;
	}

	if (m_data != NULL && NOT_OVERLAP(data, size, m_data, m_size)) {
		delete[] m_data;
	}

    m_data = new unsigned char[size];
	m_bufSize = size;
	m_size = size;
	m_start = 0;
	memcpy(m_data, data, size);
}

void Chunk::trim(int size) {
	if (0 == size || 0 == m_size) {
		return;
	}

	if (abs(size) > m_size) {
		this->clear();
		return;
	}

	if (size > 0) {
		m_start += size;
		m_size -= size;
		return;
	}

	m_size += size; // size < 0
}

Chunk::Chunk(const unsigned char *data, int size) :
	m_data(NULL),
	m_size(0),
	m_bufSize(0),
	m_start(0)
{
	copyFrom(data, size);
}

Chunk::Chunk() :
	m_data(NULL),
	m_size(0),
	m_bufSize(0),
	m_start(0)
{

}

Chunk::Chunk(const Chunk &chunk) :
	m_data(NULL),
	m_size(0),
	m_bufSize(0),
	m_start(0)
{
	copyFrom(chunk.m_data + chunk.m_start, chunk.m_size);
}

Chunk &Chunk::operator=(const Chunk &chunk) {
	if (&chunk != this) {
		copyFrom(chunk.m_data + chunk.m_start, chunk.m_size);
	}
	return (*this);
}

Chunk::~Chunk() {
	this->clear();
	if (m_data != NULL) {
		delete[] m_data;
		m_data = NULL;
		m_bufSize = 0;
	}
}

int Chunk::getSize() const {
	return m_size;
}

const unsigned char *Chunk::getData() const {
	return m_data + m_start;
}

unsigned char *Chunk::takeData() {
    unsigned char *result = m_data;
	if (m_start > 0) {
		memmove(m_data, m_data + m_start, m_size);
		m_start = 0;
	}

	m_data = NULL;
	m_size = 0;
	m_bufSize = 0;
	m_start = 0;
	return result;
}

void Chunk::trnasferTo(Chunk &chunk) {
	if (&chunk == this) {
		return;
	}

	if (chunk.m_data != NULL) {
		delete[] chunk.m_data;
	}

	chunk.m_data = m_data;
	chunk.m_bufSize = m_bufSize;
	chunk.m_size = m_size;
	chunk.m_start = m_start;

	m_data = NULL;
	m_bufSize = 0;
	m_size = 0;
	m_start = 0;
}

void Chunk::takeFrom(unsigned char *data, int size) {
	if (m_data != NULL && NOT_OVERLAP(data, size, m_data, m_size)) {
		delete[] m_data;
	}

	m_data = data;
	m_bufSize = size;
	m_size = size;
	m_start = 0;
}

void Chunk::extend(int max) {
	if (max < m_bufSize) {
		return;
	}

    unsigned char *backup = m_data;
	int start = m_start;

    m_data = new unsigned char[max];
	m_bufSize = max;
	m_start = 0;

	if (backup != NULL) {
		memcpy(m_data, backup + start, m_size);
		delete[] backup;
	}
}

void Chunk::append(const Chunk &chunk) {
	append(chunk.getData(), chunk.getSize());
}

void Chunk::append(const unsigned char *data, int size) {
	if (size < 1) {
		return;
	}

	int require = m_size + size;
	if (m_bufSize < require) {
		this->extend(m_size * 2 + size);
	}

	if (m_bufSize - m_start < require) {
		memmove(m_data, m_data + m_start, m_size);
		m_start = 0;
	}

	memcpy(m_data + m_size + m_start, data, size);
	m_size += size;
}

void Chunk::append(unsigned char data) {
	append(&data, 1);
}
