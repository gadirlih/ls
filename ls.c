#include <stdio.h>
#include <stdlib.h> //col*num_rows + row
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/stat.h>
#include <grp.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <sys/ioctl.h>

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

#ifndef TABSIZE
#define TABSIZE 8
#endif

typedef struct{
	long nentry;
	char mode[10];
	nlink_t nlinks;
	char username[BUFFSIZE];
	char groupname[BUFFSIZE];
	uid_t uid;
	gid_t gid;
	long long size;
	char time[BUFFSIZE];
	char name[BUFFSIZE];
}frecord;

typedef struct{
	int lNlinks;
	int lUsername;
	int lGroupname;
	int lUid;
	int lGid;
	int lSize;
	int lName;
	int lEntries;
}maxLen;

frecord * get_frecords();
char * parse_mode(mode_t st_mode);
char * get_username(uid_t st_uid);
char * get_groupname(gid_t st_gid);
int nDigits(long the_integer);
void print_default(frecord *records,  maxLen *max_length);
void print_result();
void parse_arguments();
void get_arguments();

int main(int argc, char **argv){
	long num_entries;
	int max_nlength;
	char cwd[BUFFSIZE];
	maxLen maxlen;
			
	frecord *records = get_frecords(argv[1], &maxlen);
	num_entries = maxlen.lEntries;
	
	//print_default(records, &maxlen);
	
	for(long nentry = 0; nentry < num_entries; nentry++){
	printf("%-10s %*d %-*s %-*s %*lld %s %-*s\n",records[nentry].mode, maxlen.lNlinks, records[nentry].nlinks, maxlen.lUsername, records[nentry].username, maxlen.lGroupname, records[nentry].groupname, maxlen.lSize, records[nentry].size, records[nentry].time, maxlen.lName, records[nentry].name);
	}
	
	return EXIT_SUCCESS;
}




frecord * get_frecords(char *path, maxLen *max_length){
	long nentry = 0;
	frecord *records;
	long long nblocks = 0;
	int fd;
	struct tm *time;
	DIR *dp;
	struct dirent *dentry;
	struct stat sbuff;
	char slinkbuff[BUFFSIZE];
	char timebuff[BUFFSIZE];
	char cwd[BUFFSIZE];
	
		
	if(chdir(path) < 0){
		perror("chdir failed");
		exit(1);
	}

	if(getcwd(cwd, BUFFSIZE) == NULL){
                perror("getcwd failed");
                exit(1);
        }

	
	if((dp = opendir(cwd)) == NULL){
		perror("Can not open directory");
		exit(1);
	}

	while((dentry = readdir(dp))!= NULL){
	
		if(lstat(dentry->d_name, &sbuff) < 0){
                       perror("lstat1 failed");
                       exit(1);
                }
		nentry++;
		nblocks += sbuff.st_blocks;
	}
	records = malloc(nentry * sizeof(frecord)); //allocate memory for records
	printf("total %lld\n", nblocks);
	rewinddir(dp);

	int max = 0;
	int link_max_len = 0;
	int size_max_len = 0;
	int username_max_len = 0;
	int groupname_max_len = 0;
	int uid_max_len = 0;
	int gid_max_len = 0;
	nentry = 0;
	while((dentry = readdir(dp)) != NULL){
		if(lstat(dentry->d_name, &sbuff) < 0){
			perror("lstat2 failed");
			exit(1);
		}
	 
		time = localtime(&sbuff.st_mtim.tv_sec);
		strftime(timebuff, BUFFSIZE, "%3b %d %H:%M", time);
		strcpy(records[nentry].time, timebuff);
		nlink_t nlinks = sbuff.st_nlink;
		records[nentry].nlinks = nlinks;
		off_t size = sbuff.st_size;
		records[nentry].size = size;
		char *username = get_username(sbuff.st_uid);
		strcpy(records[nentry].username, username);
		char *mode = parse_mode(sbuff.st_mode);
		strcpy(records[nentry].mode, mode);
		char *groupname = get_groupname(sbuff.st_gid);
		strcpy(records[nentry].groupname, groupname);
		strcpy(records[nentry].name, dentry->d_name);
		records[nentry].uid = sbuff.st_uid;
		records[nentry].gid = sbuff.st_gid;
		if(S_ISLNK(sbuff.st_mode)){
                        int n;
                        if((n = readlink(dentry->d_name, slinkbuff, BUFFSIZE)) < 0){
                                perror("readlink failed");
                                exit(1);
                        }

                        slinkbuff[n] = '\0';
			strcat(records[nentry].name, " -> ");
                        strcat(records[nentry].name, slinkbuff);
                }
		int name_length = strlen(records[nentry].name);
		if(name_length > max){
			max = name_length;
		}
		int len;
		if((len = strlen(records[nentry].username)) > username_max_len){
			username_max_len = len;
		}
		if((len = strlen(records[nentry].groupname)) > groupname_max_len){
                        groupname_max_len = len;
                }
		if((len = nDigits(records[nentry].nlinks)) > link_max_len){
                        link_max_len = len;
                }
		if((len = nDigits(records[nentry].size)) > size_max_len){
                        size_max_len = len;
                }
		if((len = nDigits(records[nentry].uid)) > uid_max_len){
                        uid_max_len = len;
                }
		if((len = nDigits(records[nentry].gid)) > gid_max_len){
                        gid_max_len = len;
                }

		
		nentry++;

		/*printf("%-11s%2d %-9s%-9s%10lld %s %-20s\n",records[nentry].mode, records[nentry].nlinks, records[nentry].username, records[nentry].groupname, records[nentry].size, records[nentry].time, records[nentry].name);*/
	}
	max_length->lEntries = nentry;
	max_length->lName = max;
	max_length->lNlinks = link_max_len;
	max_length->lUsername = username_max_len;
	max_length->lGroupname = groupname_max_len;
	max_length->lSize = size_max_len;
	max_length->lUid = uid_max_len;
	max_length->lGid = gid_max_len;

	

	return records;
}

void print_result(){
	parse_arguments();
}

void parse_arguments(){
	get_arguments();
}

void get_arguments(){
}
        
void print_default(frecord *records, maxLen *maxlen){
	struct winsize w;
        ioctl(0, TIOCGWINSZ, &w);
        int width = w.ws_col;

        int n = maxlen->lEntries;
	int max_name_len = maxlen->lName;
        int num_tabs = (max_name_len + TABSIZE -1) / TABSIZE;
        int col_width = num_tabs * TABSIZE;
        int cols = width / col_width;
        int rows = (n + cols - 1) / cols;

        for(int r = 0; r < rows; r++){
                for(int c = 0; c < cols; c++){

                        int index = c * rows + r;
                        if(index >= n) continue;
                        //index = id[index];

                        int len = strlen(records[index].name);
                        int tabs_needed = (col_width - len + TABSIZE - 1) / TABSIZE;

                        printf("%s", records[index].name);
                        while(tabs_needed--) putchar('\t');
                }
                putchar('\n');
        }


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

int nDigits(long number){
	if(number != 0){
		return (int)floor(log10(abs(number))) + 1;
	}else{
		return 1;
	} 
}
