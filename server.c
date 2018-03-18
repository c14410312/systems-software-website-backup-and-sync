#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mqueue.h>

int main(int argc, char **argv){
	
	//SETUP THE SERVER
	mqd_t mq;
	struct mq_attr queue_attributes;
	char buffer[1024 + 1];
	int terminate = 0;

	//set the attributes for the queue
	queue_attributes.mq_flags = 0;
	queue_attributes.mq_maxmsg = 10;
	queue_attributes.mq_msgsize = 1024;
	queue_attributes.mq_curmsgs = 0;

	//this creates the queue
	mq = mq_open("/webserver_queue", O_CREAT | O_RDONLY, 0644, &queue_attributes);
	
	do{
		ssize_t bytes_read;
		//RX message
		bytes_read = mq_receive(mq, buffer, 1024, NULL);
		
		buffer[bytes_read] = '\0';
		if(! strncmp(buffer, "exit", strlen("exit"))){
			terminate = 1;
		}else{
			printf("Recieved: %s\n", buffer);
		}
	}while(!terminate);

	mq_close(mq);
	mq_unlink("/webserver_queue");
	return 0;
}
