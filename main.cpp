#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <cstring>
#include <string>
#include <time.h>

#include "UDTP.hpp"

using namespace std;

int main(int argc, char* argv[]){
 
	cout	<< "Please choose an option:\n"
			<< "1) Start server.\n"
			<< "2) Start client.\n"
			<< ":";

	char choice;
	choice = cin.get();
	if(choice == '1')
	{	
		UDTP myServer;
		myServer.startListenServer();
		myServer.killListenServer();
	}
	else if(choice == '2')
	{
		UDTP myClient;
		myClient.connectTo("localhost");
		myClient.requestFile("test.png");
		myClient.disconnect();
	}
	else
	{
		cout << "\nInvalid selection.  Existing program." << endl;
	}
}
