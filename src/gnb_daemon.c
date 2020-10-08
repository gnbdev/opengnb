#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gnb_platform.h"

#ifdef __UNIX_LIKE_OS__

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#endif


#ifdef __UNIX_LIKE_OS__

int gnb_daemon(){

  int ret;

  int fd;

  pid_t pid;

  pid = fork();

  if ( 0 != pid ){
	  exit(0);
  }

  setsid();

  signal(SIGHUP,SIG_IGN);

  pid = fork();

  if ( 0 != pid ){
	  exit(0);
  }

  ret = chdir("/");

  umask(0);

  fd = open("/dev/null", O_RDWR);

  if ( -1 == fd ){
	  return -1;
  }

  ret = dup2(fd, STDIN_FILENO);

  if ( -1 == ret ) {
      return -2;
  }

  ret = dup2(fd, STDOUT_FILENO);

  if ( -1 == ret ) {
      return -3;
  }

  return 0;

}


void save_pid(const char *pid_file){

	FILE *file;

	file = fopen(pid_file,"w");

	if (NULL==file){
		printf("open pid file[%s] err\n",pid_file);
		exit(1);
	}

	int pid = getpid();

	fprintf(file,"%d",pid);

	fclose(file);

}

#endif
