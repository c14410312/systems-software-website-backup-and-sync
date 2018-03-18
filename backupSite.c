#include <stdio.h>

#define ROOT "~/Documents/systSoft/assignment1/intranetSite/"
#define DESTINY "~/Documents/systSoft/assignment1/backupFolder/"

int backupWebsite(){

	//logic to backup the website
	int a = system("rsync -raz --delete " ROOT " " DESTINY);
	if(a == -1){
		//unsuccessful
		return -1;
	}else{
		//successful
		return 1;
	}
}
