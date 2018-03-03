#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/stat.h>
#include <grp.h>
#include <sys/types.h>
#include <time.h>

#define NELEMS(x) sizeof(x)/sizeof((x)[0])
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

#ifndef BUFFSIZE
#define BUFFSIZE 4096
#endif

void l();
char * parse_mode(mode_t st_mode);
char * get_username(uid_t st_uid);
char * get_groupname(gid_t st_gid);

int main(int argc, char **argv){
	
	int fd;
	struct tm *time;
	DIR *dp;
	struct dirent *dentry;
	struct stat sbuff;
	char cwd[BUFFSIZE];
	char slinkbuff[BUFFSIZE];
	char timebuff[BUFFSIZE];

	if(getcwd(cwd, BUFFSIZE) == NULL){
		perror("getcwd failed");
		exit(1);
	}
	
	printf("cwd: %s\n", cwd);
	
	if((dp = opendir(cwd)) == NULL){
		perror("Can not open directory");
		exit(1);
	}

	

	while((dentry = readdir(dp)) != NULL){
		if(lstat(dentry->d_name, &sbuff) < 0){
			perror("lstat failed");
			exit(1);
		}
		time = localtime(&sbuff.st_mtim.tv_sec);
		strftime(timebuff, BUFFSIZE, "%3b %d %H:%M", time);
		nlink_t nlinks = sbuff.st_nlink;
		off_t size = sbuff.st_size;
		char *username = get_username(sbuff.st_uid);
		char *mode = parse_mode(sbuff.st_mode);
		char *groupname = get_groupname(sbuff.st_gid);
		printf("%-11s%2d %-9s%-9s%10lld %s %-20s\n", mode, nlinks, username, groupname, size, timebuff, dentry->d_name);
		if(S_ISLNK(sbuff.st_mode)){
			int n;	
			if((n = readlink(dentry->d_name, slinkbuff, BUFFSIZE)) < 0){
				perror("readlink failed");
				exit(1);
			}
			
			slinkbuff[n] = '\0';
			printf("%s -> %s\n", dentry->d_name, slinkbuff);
		}
	}
	
	if((fd = open(argv[1], O_RDWR)) < 0){
		perror("Unable to open the file!");
		exit(1);
	} 

	lseek(fd, 0, SEEK_SET);	

	return EXIT_SUCCESS;
}
        

char * parse_mode(mode_t st_mode){

	static char mode[10];
	if(S_ISREG(st_mode)){
		mode[0] = '-';
	}else if(S_ISDIR(st_mode)){
		mode[0] = 'd';
	}else if(S_ISCHR(st_mode)){
                mode[0] = 'c';
        }else if(S_ISBLK(st_mode)){
                mode[0] = 'b';
        }else if(S_ISFIFO(st_mode)){
                mode[0] = 'p';
        }else if(S_ISLNK(st_mode)){
                mode[0] = 'l';
        }else if(S_ISSOCK(st_mode)){
                mode[0] = 's';
        }

	if(st_mode & S_IRUSR){
		mode[1] = 'r';
	}else {
		mode[1] = '-';
	}
	if(st_mode & S_IWUSR){
                mode[2] = 'w';
        }else {
                mode[2] = '-';
        }
	if(st_mode & S_IXUSR){
		if(st_mode & S_ISUID){
			mode[3] = 's';
		}else{
                	mode[3] = 'x';
		}
        }else {
		if(st_mode & S_ISUID){
			mode[3] = 'S';
		}else{
                	mode[3] = '-';
		}
        }
	if(st_mode & S_IRGRP){
                mode[4] = 'r';
        }else {
                mode[4] = '-';
        }
	if(st_mode & S_IWGRP){
                mode[5] = 'w';
        }else {
                mode[5] = '-';
        }
	if(st_mode & S_IXGRP){
		if(st_mode & S_ISGID){
			mode[6] = 's';
		}else{
                	mode[6] = 'x';
		}
        }else {
		if(st_mode & S_ISGID){
			mode[6] = 'S';
		}else{
                	mode[6] = '-';
		}
        }
	if(st_mode & S_IROTH){
                mode[7] = 'r';
        }else {
                mode[7] = '-';
        }
	if(st_mode & S_IWOTH){
                mode[8] = 'w';
        }else {
                mode[8] = '-';
        }
	if(st_mode & S_IXOTH){
		if(st_mode & S_ISVTX){
			mode[9] = 't';
		}else{
                	mode[9] = 'x';
		}
        }else {
		if(st_mode & S_ISVTX){
			mode[9] = 'T';
		}else{
                	mode[9] = '-';
		}
        }

		
	return mode;
}

char * get_username(uid_t st_uid){
	
	struct passwd *pwentry;

	if((pwentry = getpwuid(st_uid)) == NULL){
                        perror("getpwuid failed");
                        exit(1);
        }

	return pwentry->pw_name;
}

char * get_groupname(gid_t st_gid){

	struct group *grentry;
	
	if((grentry = getgrgid(st_gid)) == NULL){
                        perror("getgrgid failed");
                        exit(1);
        }

        return grentry->gr_name;


}
