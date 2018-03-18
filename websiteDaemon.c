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
#include "syncSite.h"

//function handler for backup and sync signal
void backup_sync_handler(int signo){
	if(signo == SIGUSR1){
		//this will call the backup and sync processes
	}
}

int main(int argc, char **argv){

	time_t now;
	struct tm logtime;
	double seconds;
	time(&now); //gets the current time
	logtime = *localtime(&now);
	logtime.tm_hour = 17;
	logtime.tm_min = 06;
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
		openlog("SYSTSOFT_ASSIGN1_WEBSITE", LOG_PID | LOG_CONS, LOG_USER);


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

				//log start process
				syslog(LOG_INFO, "Backup and sync initiating");

				//send a message to message queue
				mq_send(mq, "Begin backup and sync", 1024, 0);
				sleep(2);
				//read only
				char read_only_mode[] = "0444";
				//alter the permissions for the intranet site
				//cannot write/modify while backup process taking place
				char buff[100] = "/home/dbutler/Documents/systSoft/assignment1/intranetSite";
				int ro;
				ro = strtol(read_only_mode, 0, 8);
				//if successfully changed permissions begin backup
				mq_send(mq, "Attempting to alter intranet folder permissions to read-only", 1024, 0);
				if (chmod (buff, ro) != 0){
					//error changing permissions
					mq_send(mq, "Error changing intranet directory permissions", 1024, 0);
					syslog(LOG_INFO, "Error changing intranet directory permissions");
					
				}else{
					mq_send(mq, "Intranet set to read-only", 1024, 0);
					syslog(LOG_INFO, "Permissions successfully changed for intranet directory");
					//BEGIN THE BACKUP AND SYNC PROCESS
					//STEP 1:
					mq_send(mq, "preparing to backup intranet site ...", 1024, 0);
					sleep(2);
					if(backupWebsite() == -1){
						//send error to message queue
						mq_send(mq, "Error occurred during backup", 1024, 0);
						syslog(LOG_INFO, "Error backing up website");
					}else{
						//send success message to queue
						mq_send(mq, "Website backup completed successfully", 1024, 0);
						syslog(LOG_INFO, "Website backedup successfully");
					}
						
					//STEP 2:
					mq_send(mq, "preparing to sync with livesite ...", 1024, 0);
					sleep(2);
					if(syncWebsite() == -1){
						//deal with error
						mq_send(mq, "livesite failed to sync", 1024, 0);
						syslog(LOG_INFO, "livesite failed to sync");

					}else{
						//else successfully completed
						mq_send(mq, "livesite sync completed successfully", 1024, 0);
						syslog(LOG_INFO, "livesite sync completed successfully");
					}

					mq_send(mq, "preparing to revert permissions", 1024, 0);
					sleep(2);
					//STEP 3:
					//ON COMPLETION - CHANGE FOLDER PERMISSIONS BACK
					char initial_mode[] = "0775";
					int initial;
					initial = strtol(initial_mode, 0, 8);
					if (chmod (buff, initial) != 0){
						//error changing permissions
						mq_send(mq, "Error reverting intranet directory permissions", 1024, 0);
						syslog(LOG_INFO, "Error reverting intranet directory permissions");
					}else{
						//successfully altered permissions
						mq_send(mq, "Successfully reverted intranet directory permissions", 1024, 0);
						syslog(LOG_INFO, "Successfully reverted intranet directory permissions");
					}
				}
				//log start process
				syslog(LOG_INFO, "Backup and sync completed");

				//send a message to message queue
				mq_send(mq, "Backup and sync completed", 1024, 0);
			}
		}
	}

	return 0;
}
