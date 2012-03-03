#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdint.h>

#include "common.h"
#include "bbdef.h"

#define BUFFSIZE 255

int main(void)
{
	int s;
	struct sockaddr_un remote;
	uint8_t buf[BUFFSIZE];

	memset(&remote, 0, sizeof(struct sockaddr_un));
	memset(&buf, 0, BUFFSIZE);

	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	log("Trying to connect...\n");

	remote.sun_family = AF_UNIX;
	strncpy(remote.sun_path, CS_SOCK_PATH, sizeof(remote.sun_path));
	if (connect(s, (struct sockaddr *)&remote, sizeof(struct sockaddr_un)) == -1) {
		perror("connect");
		exit(1);
	}

	log("Connected.\n");

	//this is like a test case
	////////////////////////////////////
	buf[0] = 0;
	send(s, buf, 1, 0);
	//server gets invalid opcode
	////////////////////////////////////


/* dont need this
	////////////////////////////////////
	buf[0] = OP_GET_EVENT_LIST;
	send(s, buf, 1, 0);
	//sent request for event list


	//try to recieve that
	read(s, buf, 2);
	log("[%#x][%#x]...\n", buf[0], buf[1]);
	read(s, buf+2, buf[1]);
	log("[%#x][%#x][%#x][%#x][%#x][%#x]...\n",
		   buf[0], buf[1],
		buf[2], buf[3], buf[4], buf[5]);
	////////////////////////////////////
*/
	//now try to register for something
	buf[0] = OP_REG_EVENT;
//	buf[1] = 1;
	*(uint32_t *)(buf+1) = (uint32_t)1;
	*(uint32_t *)(buf+5) = 0x12345678;
	send(s, buf, 9, 0);

	while(1)
	{
		int rcv = recv(s, buf, 1, 0);

		if(rcv < 0)
		{
			log("read error\n");
			break;
		}
		else if(rcv == 0)
		{
			log("Remote Host Closed Connection\n");
			break;
		}
		else
		{
			if(buf[0] == OP_SEND_CALLBACK)//callback
			{
				recv(s, buf+1, 4, 0);
				log("CALLBACK: %#x\n", *(uint32_t  *)(buf+1));
			}
			else
			{
				log("invalid opcode!!!!! [%#x]\n", buf[0]);
			}
		}
	}

	close(s);

	return 0;
}
