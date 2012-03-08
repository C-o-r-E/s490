#include "csSrv.h"
#include "fcntl.h"

//vector<pthread_t> threadList;

extern bbThread * threadManager;

void * runCSServer(void * arg)
{
	int s, s2, t;
	struct sockaddr_un local, remote;

	memset(&local, 0, sizeof(sockaddr_un));
	memset(&remote, 0, sizeof(sockaddr_un));

	//ask the OS for a socket
	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if(s == -1)
	{
		perror("socket");
		exit(1);
	}

	local.sun_family = AF_UNIX;
	strncpy(local.sun_path, CS_SOCK_PATH, sizeof(local.sun_path));
	unlink(local.sun_path);
	
	//bind it to our domain socket
	if (bind(s, (struct sockaddr *)&local, sizeof(sockaddr_un)) == -1) 
	{
		perror("bind");
		exit(1);
	}

	//start accepting connections
	if (listen(s, 4) == -1) 
	{
		perror("listen");
		exit(1);
	}

	t = sizeof(remote);
	while(1)
	{
		fprintf(stderr, "Waiting for a (CS) connection...\n");
		if ((s2 = accept(s, (struct sockaddr *)&remote, (socklen_t*)&t)) == -1) 
		{
			perror("accept");
			exit(1);
		}
		fprintf(stderr, "Got connection [%#x].\n", s2);

		threadManager->createDetached(handleCSConnection, (void *)&s2);
/*
		pthread_t pt;
		threadList.push_back(pt);

		pthread_create(
			&threadList[threadList.size()-1],
			NULL,
			handleCSConnection,

			(void *)&s2
			);
		pthread_detach(threadList[threadList.size()-1]);
*/
	}
}


void * handleCSConnection(void * socket)
{
	int sockFD;
	sockFD = *(int *)socket;

	uint8_t buffer[BUFFSIZE]; // buffer for received data
	
	memset(&buffer, 0, BUFFSIZE);
	
	//opcode handling state machine
	while(1)
	{
		//lets try reading the op code
		fprintf(stderr, "read opcode...\n");
		int rcv = read(sockFD, buffer, 1);

		if(rcv < 0)
		{
			perror("read error");
			break;
		}
		else if(rcv == 0)
		{
			fprintf(stderr, "Remote Host Closed Connection\n");
			break;
		}
		else
		{
			fprintf(stderr, "recieved %d bytes from [%#x]\n", rcv, sockFD);

			//actually handle commands
			uint8_t opcode = *buffer;

			if(opcode == OP_GET_EVENT_LIST)//to be removed
			{
				fprintf(stderr, "[%#x] requested event list\n", sockFD);
				//do this with the common buffer for now
				buffer[0] = OP_SEND_EVENT_LIST;
				buffer[1] = (uint8_t)knowledgeItems->size();

				for(unsigned int i=0; i<knowledgeItems->size(); ++i)
				{
					fprintf(stderr, "%d\n", (*knowledgeItems)[i]->id);
					buffer[2+i] = (uint8_t)(*knowledgeItems)[i]->id;
				}
				send(sockFD, buffer, knowledgeItems->size()+2, 0);
			}
			else if(opcode == OP_REG_EVENT)
			{
				//8bits opcode
				//32bits event tag
				//32bits addr
				read(sockFD, buffer+1, 5);
				uint8_t eId = buffer[1];
				uint32_t cbAddr = *(uint32_t *)(buffer + 2);
				fprintf(stderr, "[%#x] requested event registration\n", sockFD);
				fprintf(stderr, "\t event %d, addr[%#x]\n", eId, cbAddr);
				
				for(unsigned int i=0; i<knowledgeItems->size(); ++i)
				{
					//check if the event is valid
					if((*knowledgeItems)[i]->id == eId)
					{
						//add a listener
						fprintf(stderr, "event found...\n");
						(*knowledgeItems)[i]->addListenerOnSock(cbAddr, sockFD);
						fprintf(stderr, "callback added!\n");
					}
				}	
			}
			else if(opcode == OP_GET_LAST)
			{
				uint32_t tag;
				uint32_t num;
				read(sockFD, buffer+1, 8);
				tag = *(uint32_t *)(buffer+1);
				num = *(uint32_t *)(buffer+5);
				fprintf(stderr, "[%#x] requested last %d dataPoints for tag %#x.\n", sockFD, num, tag);

				//is the tag valid?
				if(tagMap.count(tag) > 0)
				{
					knowledgeItem * ki = tagMap[tag];
					buffer[0] = OP_RET_LAST;
					send(sockFD, buffer, 1, 0);
					ki->sendLastNdataPoints(sockFD, num);
				}
				else
				{
					fprintf(stderr, "tag %#x does not exist in database!\n", tag);
				}
			}
			else
			{
				fprintf(stderr, "invalid opcode!!!!! [%#x]\n", opcode);
			}
		}
	}

	//we lost the connection...
	//need to remove all listeners on the socket
	for(unsigned int i=0; i<knowledgeItems->size(); ++i)
	{
		(*knowledgeItems)[i]->removeListenersOnSock(sockFD);
	}
	
	fprintf(stderr, "closing socket %#x\n", sockFD);
	close(sockFD);

	threadManager->removeSelf();
/*
	for(vector<pthread_t>::iterator i=threadList.begin(); i<threadList.end(); ++i)
	{
		if(*i == pthread_self())
		{
			threadList.erase(i);
			break;
		}
	}
*/

	return NULL;
}
