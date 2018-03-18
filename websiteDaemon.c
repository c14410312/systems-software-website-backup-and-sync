//ORPHAN EXAMPLE
//The child process is adopted by init when the parent dies

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <mqueue.h>

#include "backupSite.h"

//function handler for backup and sync signal
void backup_sync_handler(int signo){
	if(signo == SIGUSR1){
		//this will call the backup and sync processes
		printf("Backup and sync\n");
	}
}

int main(int argc, char **argv){

	time_t now;
	struct tm logtime;
	double seconds;
	time(&now); //gets the current time
	logtime = *localtime(&now);
	logtime.tm_hour = 13;
	logtime.tm_min = 59;
	logtime.tm_sec = 0;
	
	//create the message queue
	mqd_t mq;
	char buffer[1024];
	
	//creates the child process
	int pid = fork();

	//if parent - kill it
	if( pid > 0){
		printf("Parent process");
		exit(EXIT_SUCCESS); //this kills the parent - makes orphan
	}
	else if(pid == 0){ //the child process
		
		//1-create the orphan process
		printf("Child Process:");

		//2-Evaluate the orphan process to session leader
		//this command runs the process in a new session
		if (setsid() < 0){
			exit(EXIT_FAILURE);
		}

		//3- call umask() to set the file mode creation to 0
		//this allows the daemon to read and write to files
		//with the correct permissions and access required
		umask(0);

		//4-Change the current working directory to root
		//this eliminates issues of running on a mounted drive
		//also eliminates the daemon doesn't keep any dir in use
		if(chdir("/") < 0) {
			exit(EXIT_FAILURE);
		}

		//5-close all the file descriptors
		//close() fds 0, 1, 2
		//releasing the IO connections inherited from the parent
		//sysconf() to determine the limit
		//_SC_OPEN_MAX --> max number of file descriptors
		//loop through and close all open fd's
		int x;
		for(x = sysconf(_SC_OPEN_MAX); x >= 0; x--){
			close(x);
		}

		//Open up the message queue
		mq = mq_open("/webserver_queue", O_WRONLY);
		mq_send(mq, "Daemon Successfully Created", 1024, 0);

		//////////////////////////
		//SIGNAL HANDLER GOES HERE
		//////////////////////////
		if(signal(SIGUSR1, backup_sync_handler) == SIG_ERR){
			printf("\nCan't catch SIGUSR1\n");
		}
		//////////////////////////
		//LOG FILE GOES HERE
		//////////////////////////


		/////////////////////////
		//ORPHAN LOGIC
		/////////////////////////
		while(1){

			//COUNTS DOWN TO A CERTAIN TIME
			sleep(1);
			time(&now);
			seconds = difftime(now, mktime(&logtime));
			printf("\n%.f", seconds);

			//IF TIMER IS 0 - PERFORM BACKUP AND UPDATE
			if(seconds==0){

				//send a message to message queue
				mq_send(mq, "Begin backup and sync", 1024, 0);

				char mode[] = "0000";
				//alter the permissions for the intranet site
				//cannot write/modify while backup process taking place
				char buff[100] = "/home/dbutler/Documents/sysSoft/assignment1/intranetSite/";
				int i;
				i = strtol(mode, 0, 8);
				//if successfully changed permissions begin backup
				if (chmod (buff, i)){
						
					//BEGIN THE BACKUP AND SYNC PROCESS
					//STEP 1:
					if(backupWebsite() == -1){
						//send error to message queue
						mq_send(mq, "Error occurred during backup", 1024, 0);
					}else{
						//send success message to queue
						mq_send(mq, "Website backup completed successfully", 1024, 0);
					}
						
					//STEP 2:
					//syncWebsite()
						
					//STEP 3:
					//ON COMPLETION - CHANGE FOLDER PERMISSIONS BACK
				}
			}
		}
	}

	return 0;
}
