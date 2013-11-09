/*
 ** UDTPFile.hpp
 **
 **  Created on: Sep 26, 2013
 **      Author: jholdsclaw
 **/

#ifndef UDTPFILE_H
#define UDTPFILE_H

#include <vector>
#include <iostream>
#include <fstream>

#define SPLIT_SIZE 420

#ifndef UDTP_MAX_FILENAME_LEN
#define UDTP_MAX_FILENAME_LEN 256
#endif

using namespace std;

// initial packet to be sent to client with file info
struct UDTPFileHeader {
    unsigned int    uiSize;             // total file size
    unsigned int    nChunks;            // number of chunks
    char            szFileName[UDTP_MAX_FILENAME_LEN];    // file name
};

// inidividual packet chunks to be sent to client
struct UDTPFileChunk {
    unsigned int    id;                 // numerical id
    unsigned int    uiChunkSize;        // size of individual chunk
    char*           buffer;             // data buffer

};

class UDTPFile {
private:
    unsigned int        m_uiMaxChunkSize;
    UDTPFileHeader		m_fhHeader;
    
	unsigned int        m_uiActiveChunk;

    ifstream*           pFileReader;

public:
    UDTPFile();
    UDTPFile(const char* _filename);
    UDTPFile(const char* _filename, unsigned int _splitSize);
    ~UDTPFile();

	UDTPFileHeader getFileHeader();

	bool processFile(const char* _filename, unsigned int _spltSize = SPLIT_SIZE);
    bool getNextSplit(char* &_dest);
    bool getSplit(unsigned int _chunkIndex, char* &_dest);

    unsigned int getMaxChunkSize();
    void setMaxChunkSize(unsigned int);

public:
	// static method to parse a UDTPFileChunk
	static bool parseSplit(const char* _source, char* &_dest, unsigned int &_size);
};

#endif UDTPFILE_H