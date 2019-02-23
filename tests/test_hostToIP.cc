#include <iostream>
#include <netdb.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fstream>
#include <time.h>
#include <cstdlib>

using namespace std;

#define MST (-7)

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		printf("usage: %s <hostname>\n", argv[0], argv[1]);
		exit(1);
	}
		
	hostent * record = gethostbyname(argv[1]);
	if(record == NULL)
	{
		printf("%s is unavailable\n", argv[1]);
		exit(1);
	}
	in_addr * address = (in_addr * )record->h_addr;
	string ip_address = inet_ntoa(* address);

	time_t rawtime;
	tm * ptm;
	time ( &rawtime );
	ptm = gmtime ( &rawtime );
	
	cout << argv[1] << " (" << ip_address << ")\n";

	ofstream ipaddr_log("ipaddr.log", ios::app);
	ipaddr_log << (ptm->tm_hour+MST%24) << ":" << (ptm->tm_min) << " " << argv[1] << " (" << ip_address << ")" << endl;
	ipaddr_log.close();
	return 0;
}
