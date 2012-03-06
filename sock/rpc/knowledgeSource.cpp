#include "knowledgeSource.h"

knowledgeSource::knowledgeSource()
{
	initialized = false;
}

knowledgeSource::knowledgeSource(uint32_t t)
{
	setTag(t);
}

knowledgeSource::~knowledgeSource()
{
	if(s > 0)
		close(s);
}

void knowledgeSource::init()
{
	if(initialized == true)
		return;

	int len;
	struct sockaddr_un remote;

	memset(&buf, 0, BUFFSIZE - 1);

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    printf("Trying to connect...\n");

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, KS_SOCK_PATH);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(s, (struct sockaddr *)&remote, len) == -1) {
        perror("connect");
        exit(1);
    }

    printf("Connected.\n");
	
	initialized = true;
}

void knowledgeSource::setTag(uint32_t t)
{
	tag = t;
}

void knowledgeSource::reg(const char * name)
{
	buf[0] = OP_REG_KS;
	//uint32_t rTag = tag;
	
	*(uint32_t *)(buf+1) = tag;
	strncpy((char *)buf+5, name, 200);
	send(s, buf, strlen(name)+6, 0);
}

void knowledgeSource::update(uint32_t len, uint8_t * data)
{
	buf[0] = OP_KS_UPDATE;
	*(uint32_t *)(buf+1) = tag;
	*(uint32_t *)(buf+5) = len;
	send(s, buf, 9, 0);
	send(s, data, len, 0);
}