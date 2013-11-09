
/*
 * file.cpp
 *
 *  Created on: Sep 26, 2013
 *      Author: jholdsclaw
 */

#include "UDTPFile.hpp"

#include <string.h>
#include <memory>

#ifndef SPLIT_SIZE
#define SPLIT_SIZE 420
#endif

UDTPFile::UDTPFile(void)
{
    m_uiMaxChunkSize = SPLIT_SIZE;
}

UDTPFile::UDTPFile(const char* _filename)
{
    processFile(_filename,SPLIT_SIZE);
}

UDTPFile::UDTPFile(const char* _filename, unsigned int _splitSize)
{
    processFile(_filename, _splitSize);
}

UDTPFile::~UDTPFile()
{
}

bool UDTPFile::processFile(const char* _filename, unsigned int _spltSize)
{
    m_uiMaxChunkSize = _spltSize;

    // clear out any ugly memory in m_fhHeader
    memset(&m_fhHeader,0,sizeof(m_fhHeader.szFileName));
    
    // open file for reading	
    pFileReader = new ifstream(_filename, ios::binary);
    
    // check that file opened and exists
    if(pFileReader->is_open()){
        // store filename for transmission
        memcpy(m_fhHeader.szFileName,_filename,strlen(_filename));
        
        // seek to end for file size
        pFileReader->seekg(0, pFileReader->end);
        m_fhHeader.uiSize = (unsigned int)pFileReader->tellg();
        
        // determine # of chunks needed
        m_fhHeader.nChunks = (unsigned int)(pFileReader->tellg()/m_uiMaxChunkSize);
        
        // return pointer to start of file
        pFileReader->seekg(0, pFileReader->beg);
        
        // set active chunk to 0
        m_uiActiveChunk = 0;
	    return true;
        
    }
    return false;
}

UDTPFileHeader UDTPFile::getFileHeader()
{
	return m_fhHeader;
}

// gets the next chunk of data and builds a "split" struct with the id, pure data chunk size, and data buffer
bool UDTPFile::getNextSplit(char* &_dest)
{
	// make sure we're not at end
    if(m_uiActiveChunk <= m_fhHeader.nChunks)
    {
		UDTPFileChunk chunk;
        chunk.id = m_uiActiveChunk;
        
        // determine if this split will be max chunk size or remainder for last packet
        chunk.uiChunkSize = (unsigned int)(m_uiMaxChunkSize>=(pFileReader->end - pFileReader->tellg())?m_uiMaxChunkSize:(pFileReader->end - pFileReader->tellg()));
        
        chunk.buffer = new char[chunk.uiChunkSize];
        pFileReader->read(chunk.buffer, chunk.uiChunkSize);
        
        // copy all of the elements into a new contiguous memory chunk
        unsigned int packet_size = (sizeof(chunk.id) + sizeof(chunk.uiChunkSize) + chunk.uiChunkSize);
        char* _buffer = new char[packet_size];
        
		memcpy(_buffer,&chunk.id,sizeof(chunk.id));
        memcpy(&_buffer[sizeof(chunk.id)],&chunk.uiChunkSize,sizeof(chunk.uiChunkSize));
        memcpy(&_buffer[sizeof(chunk.id)+sizeof(chunk.uiChunkSize)],chunk.buffer,chunk.uiChunkSize);
        
        memcpy(_dest,_buffer,packet_size);
        
        m_uiActiveChunk++;
        return true;
    }
    return false;
}

bool UDTPFile::getSplit(unsigned int _chunkIndex, char* &_dest)
{
	// TODO:	Write procedure to get requested chunkIndex
	//			just need to use copy basic functionality of
	//			of getNextSplit but use chunkIndex as offset
	//			for read instead of m_uiActiveChunk
	return true;
}

// _source is a UDTPFileChunk...strips out the raw file data
bool UDTPFile::parseSplit(const char* _source, char* &_dest, unsigned int &_size)
{
    unsigned int _id,_chunksize;
    
    memcpy(&_id,_source,sizeof(_id));
    memcpy(&_chunksize,&_source[sizeof(_id)],sizeof(_id));
    
    _size = _chunksize;
    
    char* _buffer = new char[_chunksize];
    memcpy(_buffer,&_source[sizeof(_id)+sizeof(_chunksize)],_chunksize);

    memcpy(_dest,_buffer,_size);
    
    return true;
}


void UDTPFile::setMaxChunkSize(unsigned int _size)
{
    m_uiMaxChunkSize = _size;
}

unsigned int UDTPFile::getMaxChunkSize()
{
    return m_uiMaxChunkSize;
}
