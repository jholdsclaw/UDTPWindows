
/*
 * UDTP.cpp
 *
 *  Created on: OCt 24, 2013
 *      Author: jholdsclaw
 */

#include "UDTP.hpp"
#include "UDTPFileTransfer.hpp"
#include "UDTPFile.hpp"

#include <iostream>

using namespace std;

UDTP::UDTP()
{
	m_bListenServerIsRunning = false;
	m_bConnectedToServer = false;
}

UDTP::~UDTP()
{
	// TODO: delete all dynamically allocated items (search for "new ")
}

// using winsocks code - need to abstract this
bool UDTP::startListenServer(char* _port, unsigned int _bufferSize)
{
	if(m_bListenServerIsRunning)
	{
		cerr << "Error, listen server is already running" << endl;
		return false;
	}

	m_szPort = _port;
	m_uiBufferSize = _bufferSize;

	int iResult;

	m_listenSocket = m_clientSocket = INVALID_SOCKET;	// set both to fail state initially

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &m_wsaData);
    if (iResult != 0) {
        cerr << "WSAStartup failed with error: " << iResult << endl;
        return false;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, m_szPort, &hints, &result);
    if ( iResult != 0 ) {
        cerr << "getaddrinfo failed with error: " << iResult << endl;
        WSACleanup();
        return false;
    }	

	// Create a SOCKET for connecting to server
    m_listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (m_listenSocket == INVALID_SOCKET) {
        cerr << "socket failed with error: " <<  WSAGetLastError() << endl;
        freeaddrinfo(result);
        WSACleanup();
        return false;
    }

    // Setup the TCP listening socket
    iResult = bind( m_listenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        cerr << "bind failed with error: " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        closesocket(m_listenSocket);
        WSACleanup();
        return false;
    }

	// no longer needed
    freeaddrinfo(result);

	// start listening
    iResult = listen(m_listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        cerr << "listen failed with error: " << WSAGetLastError() << endl;
        closesocket(m_listenSocket);
        WSACleanup();
        return false;
    }
	else
	{
		m_bListenServerIsRunning = true;
		cout << "Successfully started server. \nListening for connection on port " << m_szPort << "." << endl;
		do
		{
			// Accept a client socket
			m_clientSocket = accept(m_listenSocket, NULL, NULL);
			if (m_clientSocket == INVALID_SOCKET) {
				cerr << "accept failed with error: " << WSAGetLastError() << endl;
				closesocket(m_listenSocket);
				WSACleanup();
				return false;
			}
			else
			{
				sockaddr_in client_info;
				int size = sizeof(client_info);
				getpeername(m_clientSocket, (sockaddr*)&client_info, &size);
				cout << "Accepted client connection from: " << inet_ntoa(client_info.sin_addr) << "." << endl;
				
				// got a client, start litening for packets
				UDTP_Packet packet;
				do 
				{
					iResult = recv(m_clientSocket, (char*)&packet, m_uiBufferSize, 0);
					if (iResult > 0) {
						cout << "Bytes received: " <<  iResult << endl;
						switch(packet.PacketType)
						{
						case UDTP_MESSAGE: 
							// TODO: process confirmation messages
							break;
						case UDTP_FILE_REQUEST: 
							cout << "Recieved UDTP_FILE_REQUEST packet." << endl;
							// pass packet buffer in as a UDTP_Packet_File_Request structrure
							if(!startFileRequest((UDTP_Packet_File_Request*)packet.buffer))
							{
								// TODO: send error message back
							}
							break;
						case UDTP_FILE_HEADER: 
							//if(!processFileHeader(recvbuf, sizeof(packetType)))
							//	return false;
							break;
						case UDTP_FILE_CHUNK: 
							//if(!processFileChunk(recvbuf, sizeof(packetType)))
							//	return false;
							break;
						case UDTP_FILE_DONE: 
							//if(!finishFileRequest(recvbuf, sizeof(packetType)))
							//	return false;
							break;
						case UDTP_ERROR:
						default:
							cerr << "Error: invalid packet received. Terminating connection." << endl;
							//return false;
						}
					}
					else if (iResult == 0)
					   cout << "Connection closed by client..." << endl;
					else  {
						cerr << "recv failed with error: " << WSAGetLastError << endl;
						closesocket(m_clientSocket);
						WSACleanup();
						return false;
					}
				} while (iResult > 0);
				
				// shutdown the connection on our end since we're done
				iResult = shutdown(m_clientSocket, SD_SEND);
				if (iResult == SOCKET_ERROR) {
					cerr << "shutdown failed with error: " << WSAGetLastError() << endl;
					closesocket(m_clientSocket);
					WSACleanup();
					return false;
				}
			} // lost our client, loop back to listening for new client
			cout << "Client disconnected.  Listening for new connectionon port " << m_szPort << "." << endl;
		} while (true); // right now it's a never ending loop - have to force break (ctrl-c); we should use threads for listening
	}
	return true;
}

bool UDTP::connectTo(char* _hostname, char* _port, unsigned int _bufferSize)
{
	if(m_bConnectedToServer)
	{
		cerr << "Error, already connected to server." << endl;
		return false;
	}

	m_szHostname = _hostname;
	m_szPort = _port;
	m_uiBufferSize = _bufferSize;

	int iResult;

	m_listenSocket = m_clientSocket = INVALID_SOCKET;	// set both to fail state initially

    struct addrinfo *result = NULL;
	struct addrinfo *ptr = NULL;
    struct addrinfo hints;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &m_wsaData);
    if (iResult != 0) {
        cerr << "WSAStartup failed with error: " << iResult << endl;
        return false;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(m_szHostname, m_szPort, &hints, &result);
    if ( iResult != 0 ) {
        cerr << "getaddrinfo failed with error: " << iResult << endl;
        WSACleanup();
        return false;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        m_clientSocket = socket(ptr->ai_family, ptr->ai_socktype, 
            ptr->ai_protocol);
        if (m_clientSocket == INVALID_SOCKET) {
            cerr << "socket failed with error: " <<  WSAGetLastError() << endl;
            WSACleanup();
            return false;
        }

        // Connect to server.
        iResult = connect( m_clientSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(m_clientSocket);
            m_clientSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

	// no longer needed
    freeaddrinfo(result);

    if (m_clientSocket == INVALID_SOCKET) {
        cerr << "Unable to connect to server!" << endl;
        WSACleanup();
        return false;
    }

    return true;	
}

bool UDTP::disconnect()
{
	int iResult = shutdown(m_clientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		cerr << "shutdown failed with error: " << WSAGetLastError() << endl;
		closesocket(m_clientSocket);
		WSACleanup();
		return false;
	}

    // cleanup
    closesocket(m_clientSocket);
    WSACleanup();
}

bool UDTP::processPacket(UDTP_Packet _packet)
{
	switch(_packet.PacketType)
	{
	case UDTP_MESSAGE: 
		// TODO: process confirmation messages
		break;
	case UDTP_FILE_REQUEST: 
		cout << "Recieved UDTP_FILE_REQUEST packet." << endl;
		// pass packet buffer in as a UDTP_Packet_File_Request structrure
		if(!startFileRequest((UDTP_Packet_File_Request*)_packet.buffer))
			return false;
		break;
	case UDTP_FILE_HEADER: 
		//if(!processFileHeader(recvbuf, sizeof(packetType)))
		//	return false;
		break;
	case UDTP_FILE_CHUNK: 
		//if(!processFileChunk(recvbuf, sizeof(packetType)))
		//	return false;
		break;
	case UDTP_FILE_DONE: 
		//if(!finishFileRequest(recvbuf, sizeof(packetType)))
		//	return false;
		break;
	case UDTP_ERROR:
	default:
		cerr << "Error: invalid packet received. Terminating connection." << endl;
		//return false;
	}

	// Echo the buffer back to the sender
    //iSendResult = send( m_clientSocket, recvbuf, iResult, 0 );
    //if (iSendResult == SOCKET_ERROR) {
    //    cerr << "send failed with error: " << WSAGetLastError() << endl;
    //    closesocket(m_clientSocket);
    //    WSACleanup();
    //    return false;
    //}
    //cout << "Bytes sent: " << iSendResult << endl;
}


bool UDTP::killListenServer()
{
	// TODO: Not sure if I need any cleanup here??
	return true;
}

bool UDTP::requestFile(char* _filename)
{
	UDTP_Packet_File_Request filerequest;
	memcpy(filerequest.szFilename,_filename,sizeof(filerequest.szFilename));
	
	cout << "Requesting file: " << _filename << endl;

	UDTP_Packet packet;
	packet.PacketType = UDTP_FILE_REQUEST;
	memcpy(&packet.buffer, &filerequest, sizeof(filerequest));

	cout << "Sending request packet." << endl;
	// Send request packet
    int iResult = send( m_clientSocket, (char*)&packet, m_uiBufferSize, 0 );
	if (iResult == SOCKET_ERROR) {
        cerr << "send failed with error: " << WSAGetLastError() << endl;
        closesocket(m_clientSocket);
        WSACleanup();
        return false;
    }

	cout << "Bytes Senta: " << iResult << endl;
	
	// successfully sent request, now we wait for confirmation
	do 
	{
		cout << "Waiting for confirmation packet (UDTP_FILE_HEADER). " << endl;
		ZeroMemory(&packet,sizeof(packet));
		iResult = recv(m_clientSocket, (char*)&packet, m_uiBufferSize, 0);
		cout << "Rec'd confirmation packet (UDTP_FILE_HEADER)." << endl;
		if (iResult > 0) {
			cout << "Bytes received: " <<  iResult << endl;
			switch(packet.PacketType)
			{
			// got a successful file request confirmation
			case UDTP_FILE_HEADER: 
				if(processFileHeader((UDTPFileHeader*)packet.buffer))
				{
					cout << "Successfully received entire file. Terminating connection." << endl;
					return true;
				}
				else
				{
					cout << "Error receiving file.  Terminating connection." << endl;
					return false;
				}
				break;
			case UDTP_MESSAGE: 
				// TODO: process confirmation messages
				break;
			case UDTP_ERROR:
			default:
				cerr << "Error: invalid packet received. Terminating connection." << endl;
				//return false;
			}
		}
		else if (iResult == 0)
			cout << "Connection closed by client..." << endl;
		else  {
			cerr << "recv failed with error: " << WSAGetLastError << endl;
			closesocket(m_clientSocket);
			WSACleanup();
			return false;
		}
	} while (iResult > 0);
	
	return true;
}

bool UDTP::processFileHeader(UDTPFileHeader* _fileheader)
{
	m_pUDTPFileHeader = new UDTPFileHeader;
	memcpy(m_pUDTPFileHeader, _fileheader, sizeof(UDTPFileHeader));

	// TODO: need to refactor and remove hardcoded filename
	// only like this for initial testing
	m_Outputfile = new ofstream("new.png", ofstream::binary);

	// now we wait to receive all chunk packets
	UDTP_Packet packet;
	int iResult = 0;
	do 
	{
		iResult = recv(m_clientSocket, (char*)&packet, m_uiBufferSize, 0);
		if (iResult > 0) {
			cout << "Bytes received: " <<  iResult << endl;
			switch(packet.PacketType)
			{
			// got a successful file chunk
			case UDTP_FILE_CHUNK: 
				cout << "Rec'd file chunk.  Attempting to process" << endl;
				if(!processFileChunk(packet.buffer))
				{
					cerr << "Error processing file chunk." << endl;
					return false;
				}
				break;
			case UDTP_FILE_DONE:
				cout << "Rec'd UDTP_FILE_DONE packet." << endl;
				m_Outputfile->close();
				return true;
			case UDTP_MESSAGE: 
				// TODO: process confirmation messages
				break;
			case UDTP_ERROR:
			default:
				cerr << "Error: invalid packet received. Terminating connection." << endl;
				//return false;
			}
		}
		else if (iResult == 0)
			cout << "Connection closed by client..." << endl;
		else  {
			cerr << "recv failed with error: " << WSAGetLastError << endl;
			closesocket(m_clientSocket);
			WSACleanup();
			return false;
		}
	} while (iResult > 0);

	return true;
}

bool UDTP::processFileChunk(char* _filechunk)
{
	unsigned int _id, _size = 0;
	memcpy(&_id,_filechunk,sizeof(_id));

	char* _buffer = new char[m_uiBufferSize];

	cout << "Rec'd file chunk #" << _id << "." << endl;

	UDTPFile::parseSplit(_filechunk,_buffer,_size);

	if(m_Outputfile->is_open())
		m_Outputfile->write(_buffer,_size);

	return true;
}

bool UDTP::sendFile()
{
	UDTP_Packet packet;
	ZeroMemory(&packet,sizeof(packet));

	packet.PacketType = UDTP_FILE_CHUNK;

	char* _buffer = new char[m_pUDTPFile->getMaxChunkSize()];
	
	// keep sending the file chunk packets until done
	bool bKeepGoing = true;
	while(bKeepGoing)
	{
		// try to grab next split
		if(m_pUDTPFile->getNextSplit(_buffer))
		{
			// stuff next split into packet
			memcpy(&packet.buffer,_buffer,m_pUDTPFile->getMaxChunkSize());

			// Send filled file chunk packet
			int iResult = send( m_clientSocket, (char*)&packet, m_uiBufferSize, 0 );
			if (iResult == SOCKET_ERROR) {
				cerr << "send failed with error: " << WSAGetLastError() << endl;
				closesocket(m_clientSocket);
				WSACleanup();
				return false;
			}
		}
		else
		{
			// done sending the splits, let's finish up
			ZeroMemory(&packet,sizeof(packet));
			packet.PacketType = UDTP_FILE_DONE;
			// Send file done packet
			int iResult = send( m_clientSocket, (char*)&packet, m_uiBufferSize, 0 );
			if (iResult == SOCKET_ERROR) {
				cerr << "send failed with error: " << WSAGetLastError() << endl;
				closesocket(m_clientSocket);
				WSACleanup();
				return false;
			}

			bKeepGoing = false;
		}
	}
	return true;
}

bool UDTP::startFileRequest(UDTP_Packet_File_Request* filerequest)
{
	// TODO: check if we are expecting to start a new filerequest?

	// make sure we got a valid filename
	if(strlen(filerequest->szFilename) <= 0)
	{
		cerr << "Error: Invalid filename." << endl;
		// TODO: Send back an error message to client
	}

	cout << "UDTP::startFileRequest rec'd request for file: " << filerequest->szFilename << endl;

	m_pUDTPFile = new UDTPFile();
	if(!m_pUDTPFile->processFile(filerequest->szFilename))
	{
		cerr << "Error processing file request.  Does file exist on server?" << endl;
		return false;
	}
	

	// looks like we're good to go, send confirmation back to client
	UDTP_Packet packet;
	packet.PacketType = UDTP_FILE_HEADER;
	memcpy(&packet.buffer, &m_pUDTPFile->getFileHeader(), sizeof(m_pUDTPFile->getFileHeader()));

	// Send request packet
    int iResult = send( m_clientSocket, (char*)&packet, m_uiBufferSize, 0 );
	if (iResult == SOCKET_ERROR) {
        cerr << "send failed with error: " << WSAGetLastError() << endl;
        closesocket(m_clientSocket);
        WSACleanup();
		return false;
    }

	sendFile();

	return true;
}


