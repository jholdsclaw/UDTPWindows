/*
 ** UDTP.hpp
 **
 **  Created on: Oct 24, 2013
 **      Author: jholdsclaw
 **/

#ifndef UDTP_H
#define UDTP_H

// all winsocks code - need to abstract this
#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
// end winsocks code

#include "UDTPFile.hpp"
#include <fstream>

#include <fstream>

#define DEFAULT_BUFFER 512
#define PACKET_BUFFER_SIZE (DEFAULT_BUFFER - sizeof(unsigned int))
#define	DEFAULT_PORT "6666"
#define UDTP_MAX_FILENAME_LEN 96

using namespace std;

enum UDTP_Packet_Type
{
	UDTP_ERROR,
	UDTP_MESSAGE,
	UDTP_FILE_REQUEST,
	UDTP_FILE_HEADER,
	UDTP_FILE_CHUNK,
	UDTP_FILE_DONE
};

struct UDTP_Packet
{
	UDTP_Packet_Type	PacketType;
	char				buffer[PACKET_BUFFER_SIZE];
};

struct UDTP_Packet_File_Request
{
	char			szFilename[UDTP_MAX_FILENAME_LEN];
};


class UDTP {
private:
	char*				m_szHostname;
	char*				m_szPort;
	unsigned int		m_uiBufferSize;

	bool				m_bListenServerIsRunning;
	bool				m_bConnectedToServer;

	// TODO: refactor this later for multi-threading/multi-clients
	UDTPFile*			m_pUDTPFile;
	UDTPFileHeader*		m_pUDTPFileHeader;
	ofstream*			m_Outputfile;

	//winsocks specific code - need to abstract this
	WSADATA				m_wsaData;
	SOCKET				m_listenSocket;
	SOCKET				m_clientSocket;

public:
    UDTP();
    ~UDTP();

    bool startListenServer(char* _port = DEFAULT_PORT, unsigned int _bufferSize = DEFAULT_BUFFER);
	bool killListenServer();

	bool connectTo(char* _hostname, char* _port = DEFAULT_PORT, unsigned int _bufferSize = DEFAULT_BUFFER);
	bool disconnect();

	bool requestFile(char* _filename);
	bool processFileHeader(UDTPFileHeader* _fileheader);
	bool processFileChunk(char* _filechunk);

	bool sendFile();

	bool startFileRequest(UDTP_Packet_File_Request* _filerequest);
private:
	bool processPacket(UDTP_Packet _packet);
};

#endif UDTP_H
