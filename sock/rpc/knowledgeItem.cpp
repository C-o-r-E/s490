#include <sys/socket.h>
#include <cstring>

#include "common.h"
#include "knowledgeItem.h"
#include "bbdef.h"

dataPoint::dataPoint()
{
	size = 0;
	data = NULL;
}

dataPoint::~dataPoint()
{
	log("destroy datapoint: size=%d data at addr=%p\n", size, data);
	if(data!=NULL)
		delete[] data;
}

knowledgeItem::knowledgeItem()
{
	pthread_mutex_init(&mutex, NULL);

	pthread_mutex_lock(&mutex);
	//by default we keep one data point
	storageSize = 1;
	pthread_mutex_unlock(&mutex);
}

void knowledgeItem::setName(char * str)
{
	pthread_mutex_lock(&mutex);
	name = str;
	pthread_mutex_unlock(&mutex);
}

void knowledgeItem::update(uint32_t len, uint8_t * newData)
{
	dataPoint * d = new dataPoint();
	d->size = len;
	d->data = newData;
	pthread_mutex_lock(&mutex);
	if(dataList.size() == storageSize)
	{
		dataList.pop_front();
	}
	dataList.push_back(d);
	pthread_mutex_unlock(&mutex);
	updateListeners();
	log("Update KI [%s] with %d bytes\n", name.c_str(), len);
	log("[%s] has %d/%d dataPoints\n", name.c_str(), (int)dataList.size(), storageSize);
}

void knowledgeItem::setStorageSize(uint32_t size)
{
	log("[%s]changing storage size from %d to %d\n", name.c_str(), storageSize, size);
	pthread_mutex_lock(&mutex);
	//if the new size is smaller than current size
	//need to shrink the list
	while(size < dataList.size())
	{
		delete dataList.front();
		dataList.pop_front();
	}
	storageSize = size;
	pthread_mutex_unlock(&mutex);
	log("[%s] has %d/%d dataPoints\n", name.c_str(), (int)dataList.size(), storageSize);
}

void knowledgeItem::addListenerOnSock(uint32_t cbA, int sock)
{
	remote_callback * rcb = new remote_callback;
	rcb->socket = sock;
	rcb->addr = cbA;

	pthread_mutex_lock(&mutex);
	listeners.push_back(rcb);
	pthread_mutex_unlock(&mutex);
}

void knowledgeItem::updateListeners()
{
	uint8_t buf[16];
	remote_callback * rc;

	dataPoint * d;
	log("update listeners...\n");

	pthread_mutex_lock(&mutex);
	d = dataList.back();
	for(unsigned int i = 0; i < listeners.size(); ++i)
	{
		rc = listeners[i];
		log("callback:");
		print();
		buf[0] = OP_SEND_CALLBACK; 
		memcpy(buf+1, &(rc->addr), 4);//TODO: endian
		memcpy(buf+5, &(d->size), 4);//not sure why i used memcpy here...
		hexDump(buf, 9);
		send(rc->socket, buf, 9, 0);
		send(rc->socket, d->data, d->size, 0);
	}
	pthread_mutex_unlock(&mutex);
}

void knowledgeItem::removeListenersOnSock(int sock)
{
	pthread_mutex_lock(&mutex);
	for(deque<remote_callback *>::iterator i=listeners.begin(); i<listeners.end(); ++i)
	{
		if((*i)->socket == sock)
		{
			log("Removing listener on socket %#x from knowledgeItem %d\n", sock, id);
			remote_callback * rcb = *i;
			delete rcb;
			listeners.erase(i);
		}
	}
	pthread_mutex_unlock(&mutex);
}

//this is pretty much custom made for the OP_RET_LAST messgage
void knowledgeItem::sendLastNdataPoints(int sock, uint32_t n)
{
	log("sendLastNdataPoints N=%d\n", n);
	pthread_mutex_lock(&mutex);
	if(dataList.size() < n)
	{
		uint8_t num[4];
		*(uint32_t *)num = dataList.size();
		//send all the points we have
		send(sock, num, 4, 0);
	
		
		list<dataPoint *>::reverse_iterator rit;
		for(rit=dataList.rbegin(); rit!=dataList.rend(); ++rit)
		{
			dataPoint * tmp = *rit;
			*(uint8_t *)num = tmp->size;
			send(sock, num, 4, 0);
			send(sock, tmp->data, tmp->size, 0);
		}
		pthread_mutex_unlock(&mutex);
		return;
	}

	//otherwize we need only send n
	uint8_t num[4];
	*(uint32_t *)num = n;
	send(sock, num, 4, 0);
	
	uint32_t count = 0;
	list<dataPoint *>::reverse_iterator rit;
	for(rit=dataList.rbegin(); rit!=dataList.rend() && count < n; ++rit)
	{
		dataPoint * tmp = *rit;
		*(uint8_t *)num = tmp->size;
		send(sock, num, 4, 0);
		send(sock, tmp->data, tmp->size, 0);
		++count;
	}
	
	pthread_mutex_unlock(&mutex);
}

void knowledgeItem::print()
{
	fprintf(stderr, "knowledgeItem:%p\n", this);
	fprintf(stderr, "\tname:[%s]", name.c_str());
	fprintf(stderr, "\tstorageSize:%d\n", storageSize);
	fprintf(stderr, "\tid:%#X\n", id);
	
	for(list<dataPoint *>::iterator i=dataList.begin(); i!=dataList.end(); ++i)
	{
		fprintf(stderr, "\t\tdataPoint:%p size:%d data:%p\n", *i, (*i)->size, (*i)->data);
		fprintf(stderr, "\t\t\n");
	}
}

//Corey's fancy function for visualizing binary data
void hexDump(void * data, size_t byteLen)
{
	unsigned int line, col;
	char hex[49];
	char ascii[17];
	
	memset(hex, 0, 48);
	memset(ascii, 0, 16);
	
	if(byteLen == 0) 
		return;
	
	fprintf(stderr, "0x  00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f \n");
	for(line=0; line<(byteLen/16)+1; line++)
	{
		for(col=0; col<16; col++)
		{
			unsigned int pos = 16*line + col;
			unsigned char curByte;
			
			if(pos>=byteLen)
				break;
			
			curByte = *(((unsigned char *)(data)+pos));
			sprintf(hex, "%s%02x ", hex, curByte);
			
			if( curByte>=32 && curByte<127 )
				sprintf(ascii, "%s%c", ascii, curByte);
			else
				sprintf(ascii, "%s%s", ascii, ".");
		}
		fprintf(stderr, "%02x  %-48s %-16s\n", line, hex, ascii);
		memset(hex, 0, 48);
		memset(ascii, 0, 16);
	}
}
