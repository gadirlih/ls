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
#include <string.h>
#include <math.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <locale.h>

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

typedef int bool;
enum {false, true};

typedef struct{
	long nentry;
	char mode[10];
	nlink_t nlinks;
	char username[BUFFSIZE];
	char groupname[BUFFSIZE];
	uid_t uid;
	gid_t gid;
	long long size;
	char byte[BUFFSIZE];
	char smtime[BUFFSIZE];
	char sctime[BUFFSIZE];
	char satime[BUFFSIZE];
	long mtime;
	long ctime;
	long atime;
	long nmtime;
	long nctime;
	long natime;
	char name[BUFFSIZE];
	char nslname[BUFFSIZE];
	long blocks;
}frecord;

typedef struct{
	int lNlinks;
	int lUsername;
	int lGroupname;
	int lUid;
	int lGid;
	int lSize;
	int lByte;
	int lName;
	int lNslname;
	int lEntries;
	long lTotal;
}maxLen;

typedef struct{
	int nFiles;
	int nDirs;
	int nArgs;
}ncmd;

typedef struct{
	bool PRINT_SIMPLE;
	bool PRINT_LONG;
	bool HIDDEN_FILES;
	bool SORT_ABC;
	bool LONG_UID;
	bool NAME_SLINK;
	bool UPPER_A;
	bool LAST_MOD;
	bool LAST_ACC;
	bool LAST_CHANGE;
	bool SORT_TIME;
	bool SORT_SIZE;
	bool SORT_ABC_DIR;
	bool REVERSE;
	bool BYTE_SIZE;
}argFlags;

frecord * get_frecords();
char * parse_mode(mode_t st_mode);
char * get_username(uid_t st_uid);
char * get_groupname(gid_t st_gid);
int nDigits(long the_integer);
void print_default(frecord *records,  maxLen *max_length);
void print_result(char *files[], char *dirs[], char *args, ncmd ncom);
ncmd parse_command(char *files[], char *dirs[], char *args, int argc, char **argv);
void parse_arguments();
void get_arguments();
frecord *no_dot_records();
maxLen maxlength();
char * get_time_string();
char * byte_size();

static const char *sizes[]   = { "E", "P", "T", "G", "M", "K", "" };
static const long long exbibytes = 1024ULL * 1024ULL * 1024ULL *
                                   1024ULL * 1024ULL * 1024ULL;

argFlags flag = {.PRINT_SIMPLE = true, .PRINT_LONG = false, .HIDDEN_FILES = false, .SORT_ABC = true, .LONG_UID = false, .NAME_SLINK = false, .UPPER_A = false, .LAST_MOD = true, .LAST_ACC = false, .LAST_CHANGE = false, .SORT_TIME = false, .SORT_SIZE = false, .SORT_ABC_DIR = true, .REVERSE = false, .BYTE_SIZE = false};

int main(int argc, char **argv){
	char *files[BUFFSIZE];
	char *dirs[BUFFSIZE];
	char args[BUFFSIZE];
	ncmd ncom;
	
	ncom = parse_command(files, dirs, args, argc, argv);
	if(ncom.nFiles == 0 && ncom.nDirs == 0){
		char def[1] = {'.'};
		dirs[0] = def;
		ncom.nDirs = 1;
	}

	setlocale(LC_ALL, "en_US.UTF-8");

	print_result(files, dirs, args, ncom);

	return EXIT_SUCCESS;
}

maxLen maxlength(frecord *records, long ent){
	maxLen max_length;
	//max_length = malloc(sizeof(maxLen));
	int name_max_len = 0;
	int nslname_max_len = 0;
        int link_max_len = 0;
        int size_max_len = 0;
        int username_max_len = 0;
        int groupname_max_len = 0;
        int uid_max_len = 0;
        int gid_max_len = 0;
	int byte_max_len = 0;
        int nentry = 0;
	long total = 0;
	
	while(nentry != ent){
		
		int name_length = strlen(records[nentry].name);	
		if(name_length > name_max_len){
                	name_max_len = name_length;
        	}
        	int len;
        	if((len = strlen(records[nentry].username)) > username_max_len){
                	username_max_len = len;
        	}
		if((len = strlen(records[nentry].byte)) > byte_max_len){
                        byte_max_len = len;
                }
		if((len = strlen(records[nentry].nslname)) > nslname_max_len){
                        nslname_max_len = len;
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
		total += records[nentry].blocks;
		nentry++;
	}
	
	max_length.lNslname = nslname_max_len;
	max_length.lEntries = nentry;
        max_length.lName =name_max_len;
        max_length.lNlinks = link_max_len;
        max_length.lUsername = username_max_len;
        max_length.lGroupname = groupname_max_len;
        max_length.lSize = size_max_len;
        max_length.lUid = uid_max_len;
        max_length.lGid = gid_max_len;
	max_length.lTotal = total;
	max_length.lByte = byte_max_len;
	
	return max_length;
}

frecord * get_frecords(char *path, maxLen *max_length, bool isFile, char *files[], ncmd *ncom){
	long nentry = 0;
	frecord *records;
	
	int fd;
	struct tm smtime, sctime, satime;
	long mtime, ctime, atime;
	long nmtime, nctime, natime;
	DIR *dp;
	struct dirent *dentry;
	struct stat sbuff;
	char slinkbuff[BUFFSIZE];
	char timebuff[BUFFSIZE];
	char cwd[BUFFSIZE];
	char *byte;
	bool next_dir = false;
	long lTotal = 0;
	//smtime = malloc(sizeof(struct tm));


	if(!isFile){
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
			lTotal += sbuff.st_blocks;
			nentry++;
		}
		max_length->lTotal = lTotal;
	
		rewinddir(dp);
	}
	
	if(!isFile) {
		records = malloc(nentry * sizeof(frecord)); 	//allocate memory for records
	}else{
		records = malloc((ncom->nFiles) * sizeof(frecord));		//allocate mem for 1 file
	}

	int max = 0;
	int nslname_max_len = 0;
	int link_max_len = 0;
	int size_max_len = 0;
	int username_max_len = 0;
	int groupname_max_len = 0;
	int uid_max_len = 0;
	int gid_max_len = 0;
	int byte_max_len = 0;
	nentry = 0;

	if(!isFile){
		if((dentry = readdir(dp)) != NULL){
			next_dir = true;
		}
	}
	int f = 0;
	int file_count = 0;
	if(files != NULL){
		file_count = ncom->nFiles;
	}
	
	while(isFile || next_dir){
		
		
		if(isFile){
			if(lstat(files[f], &sbuff) < 0){
                        	perror("lstat1 failed");
                        	exit(1);
			}
			
                }else if(lstat(dentry->d_name, &sbuff) < 0){
			perror("lstat2 failed");
			exit(1);
		}
	 	byte = byte_size(sbuff.st_size);
		strcpy(records[nentry].byte, byte);
		smtime = *localtime(&sbuff.st_mtim.tv_sec);
		sctime = *localtime(&sbuff.st_ctim.tv_sec);
		satime = *localtime(&sbuff.st_atim.tv_sec);
		//strftime(timebuff, BUFFSIZE, "%3b %d %H:%M", &smtime);
		strcpy(records[nentry].smtime, get_time_string(asctime(&smtime)));
		strcpy(records[nentry].sctime, get_time_string(asctime(&sctime)));
		strcpy(records[nentry].satime, get_time_string(asctime(&satime)));
		
		records[nentry].mtime = sbuff.st_mtim.tv_sec;
		records[nentry].ctime = sbuff.st_ctim.tv_sec;
		records[nentry].atime = sbuff.st_atim.tv_sec;
		
		records[nentry].nmtime = sbuff.st_mtim.tv_nsec;
                records[nentry].nctime = sbuff.st_ctim.tv_nsec;
                records[nentry].natime = sbuff.st_atim.tv_nsec;
		
		nlink_t nlinks = sbuff.st_nlink;
		records[nentry].nlinks = nlinks;
		off_t size = sbuff.st_size;
		records[nentry].size = size;
		records[nentry].blocks = sbuff.st_blocks;
		char *username = get_username(sbuff.st_uid);
		strcpy(records[nentry].username, username);
		char *mode = parse_mode(sbuff.st_mode);
		strcpy(records[nentry].mode, mode);
		char *groupname = get_groupname(sbuff.st_gid);
		strcpy(records[nentry].groupname, groupname);
		if(!isFile){
			strcpy(records[nentry].name, dentry->d_name);
			strcpy(records[nentry].nslname, dentry->d_name);
		}else{
			strcpy(records[nentry].name, files[f]);
			strcpy(records[nentry].nslname, files[f]);
		}
		records[nentry].uid = sbuff.st_uid;
		records[nentry].gid = sbuff.st_gid;
		if(S_ISLNK(sbuff.st_mode)){
                        int n;
                        if(!isFile && ((n = readlink(dentry->d_name, slinkbuff, BUFFSIZE)) < 0)){
                                perror("readlink1 failed");
                                exit(1);
                        }else if(isFile && ((n = readlink(files[f], slinkbuff, BUFFSIZE)) < 0)){
                                perror("readlink2 failed");
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
		if((len = strlen(records[nentry].byte)) > byte_max_len){
                        byte_max_len = len;
                }
		if((len = strlen(records[nentry].nslname)) > nslname_max_len){
                        nslname_max_len = len;
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

		if(!isFile){
                	if((dentry = readdir(dp)) != NULL){
                        	next_dir = true;
                	}else{
				next_dir = false;
			}
        	}
		if(isFile){
			f++;
			if(f == file_count){
				 isFile = false;
			}
		}
		
		nentry++;
		
	}
	max_length->lNslname = nslname_max_len;
	max_length->lEntries = nentry;
	max_length->lName = max;
	max_length->lNlinks = link_max_len;
	max_length->lUsername = username_max_len;
	max_length->lGroupname = groupname_max_len;
	max_length->lSize = size_max_len;
	max_length->lUid = uid_max_len;
	max_length->lGid = gid_max_len;
	max_length->lByte = byte_max_len;
	
	
	return records;
}
char * get_time_string(char *asctime){
	char *t;
	int year;
	t = malloc(sizeof(char *) * BUFFSIZE);
	
	
	year = strtol(asctime + 20, 0, 10);

	time_t cur = time(NULL);
	struct tm *curt = localtime(&cur);
	
	if(year == curt->tm_year + 1900){
		strncpy(t, asctime + 4, 12);
	}else{
		char tmp[BUFFSIZE];
		sprintf(tmp, " %d", year);
		strncpy(t, asctime + 4, 7);
		strcat(t, tmp);
	}
	return t;
} 


ncmd parse_command(char *files[], char *dirs[], char *args, int argc, char **argv){
	DIR *dp;
	ncmd ncom;
	int f = 0;
	int d = 0;

	for(int i = 1; i < argc; i++){
		char *tmp = argv[i];
		if((dp = opendir(argv[i])) != NULL){
        		dirs[d] = argv[i];
			d++;
        	}else if(tmp[0] == '-'){
			strcat(args, tmp + 1);
		}else{
			files[f] = argv[i];
			f++;
		}
	}
	ncom.nFiles = f;
	ncom.nDirs = d;
	ncom.nArgs = strlen(args);
	return ncom;
}

int cmp_str(const void *a, const void *b){
	char *tmpl, *tmpr;
	
	frecord *l = (frecord *)a;
	frecord *r = (frecord *)b;
	
	/*if(*(l->name) == '.') tmpl = (l->name) + 1;
	else tmpl = l->name;
	
	if(*(r->name) == '.') tmpr = (r->name) + 1;
	else tmpr = r->name;
	
	for(;;tmpl++, tmpr++){
		int res = tolower(*tmpl) - tolower(*tmpr);
		if(res != 0 || !*tmpl) return res;
	}*/
		 
	return strcoll(l->name, r->name);
}

int cmp_dirs(const void *a, const void *b){
        char *tmpl, *tmpr;

        char **l = (char **)a;
	char **r = (char **)b;

        /*if(**(l) == '.') tmpl = *(l) + 1;
        else tmpl = *l;

        if(**(r) == '.') tmpr = *(r) + 1;
        else tmpr = *r;

        for(;;tmpl++, tmpr++){
                int res = tolower(*tmpl) - tolower(*tmpr);
                if(res != 0 || !*tmpl) return res;
        }*/

	return strcoll(*l, *r);
}

int cmp_mtime(const void *a, const void *b){
        char *tmpl, *tmpr;

        frecord *l = (frecord *)a;
        frecord *r = (frecord *)b;
	
	long res = l->mtime - r->mtime;
	long nano = l->nmtime - r->nmtime;
	
	if(res == 0){
		if(nano != 0) return -nano;
		else return strcoll(l->name, r->name);
	}else{ 
		return -res;
	}
	//return (res != 0)?(-res):0;
}

int cmp_ctime(const void *a, const void *b){
        char *tmpl, *tmpr;

        frecord *l = (frecord *)a;
        frecord *r = (frecord *)b;

        long res = l->ctime - r->ctime;
	long nano = l->nctime - r->nctime;

        if(res == 0){
                if(nano != 0) return -nano;
                else return strcoll(l->name, r->name);
        }else{
                return -res;
        }
}

int cmp_atime(const void *a, const void *b){
        char *tmpl, *tmpr;

        frecord *l = (frecord *)a;
        frecord *r = (frecord *)b;

        long res = l->atime - r->atime;
	long nano = l->natime - r->natime;

        if(res == 0){
                if(nano != 0) return -nano;
                else return strcoll(l->name, r->name);
        }else{
                return -res;
        }
}

int cmp_size(const void *a, const void *b){
        char *tmpl, *tmpr;

        frecord *l = (frecord *)a;
        frecord *r = (frecord *)b;

        long long res = l->size - r->size;

        return (res != 0)?-res:(strcoll(l->name, r->name));
}


int cmp_dir_atime(const void *a, const void *b){
        char *tmpl, *tmpr;

        char **l = (char **)a;
        char **r = (char **)b;

	struct stat *sbuffl;
	sbuffl = malloc(sizeof(struct stat));
	if(lstat(*l, sbuffl) < 0){
		perror("lstat in cmp_dir_atime failed");
                exit(1);
        }
	
	struct stat *sbuffr;
	sbuffr = malloc(sizeof(struct stat));
        if(lstat(*r, sbuffr) < 0){
                perror("lstat in cmp_dri_atime failed");
                exit(1);
        }

        long res = sbuffl->st_atim.tv_sec - sbuffr->st_atim.tv_sec;
        long nano = sbuffl->st_atim.tv_nsec - sbuffr->st_atim.tv_nsec;

        if(res == 0){
                if(nano != 0) return -nano;
                else return strcoll(*l, *r);
        }else{
                return -res;
        }
}

int cmp_dir_mtime(const void *a, const void *b){
        char *tmpl, *tmpr;

        char **l = (char **)a;
        char **r = (char **)b;

        struct stat *sbuffl;
	sbuffl = malloc(sizeof(struct stat));
        if(lstat(*l, sbuffl) < 0){
                perror("lstat in cmp_dir_mtime failed");
                exit(1);
        }

        struct stat *sbuffr;
	sbuffr = malloc(sizeof(struct stat));
        if(lstat(*r, sbuffr) < 0){
                perror("lstat in cmp_dri_mtime failed");
                exit(1);
        }

	long res = sbuffl->st_mtim.tv_sec - sbuffr->st_mtim.tv_sec;
        long nano = sbuffl->st_mtim.tv_nsec - sbuffr->st_mtim.tv_nsec;

        if(res == 0){
                if(nano != 0) return -nano;
                else return strcoll(*l, *r);
        }else{
                return -res;
        }
}

int cmp_dir_ctime(const void *a, const void *b){
        char *tmpl, *tmpr;

        char **l = (char **)a;
        char **r = (char **)b;

        struct stat *sbuffl;
	sbuffl = malloc(sizeof(struct stat));
        if(lstat(*l, sbuffl) < 0){
                perror("lstat in cmp_dir_ctime failed");
                exit(1);
        }

        struct stat *sbuffr;
	sbuffr = malloc(sizeof(struct stat));
        if(lstat(*r, sbuffr) < 0){
                perror("lstat in cmp_dri_ctime failed");
                exit(1);
        }

	long res = sbuffl->st_ctim.tv_sec - sbuffr->st_ctim.tv_sec;
        long nano = sbuffl->st_ctim.tv_nsec - sbuffr->st_ctim.tv_nsec;

        if(res == 0){
                if(nano != 0) return -nano;
                else return strcoll(*l, *r);
        }else{
                return -res;
        }
}



frecord *no_dot_records(frecord *records, long recent, maxLen *maxlen){
	
	frecord *nodot;
	nodot = malloc(recent * sizeof(frecord));
	long nodot_ent = 0;
	for(int i = 0; i < recent; i++){
		if(*(records[i].name) != '.'){
			nodot[nodot_ent] = records[i];
			nodot_ent++;
		}
	}
	*maxlen = maxlength(nodot, nodot_ent);
	return nodot;
}

frecord *reverse_record(frecord *records, long nentry){
	frecord *rev;
	rev = malloc(nentry * sizeof(frecord));
	long n = 0;
	
	for(int i = nentry - 1; i >= 0; i--){
		rev[n] = records[i];
		n++;
	}
	
	return rev;
}

char * byte_size(long long size)
{   
    char * result = (char *) malloc(sizeof(char) * 20);
    long long multiplier = exbibytes;
    
    for (int i = 0; i < sizeof(sizes)/sizeof(*(sizes)); i++, multiplier /= 1024)
    {   
        if (size < multiplier)
            continue;
        if (size % multiplier == 0){
            	if(i != 6) sprintf(result, "%lld.0%s", size / multiplier, sizes[i]);
		else sprintf(result, "%lld%s", size / multiplier, sizes[i]);
        }else{
            sprintf(result, "%.1Lf%s", (long double) size / multiplier + 0.05, sizes[i]);
        }
	return result;
    }
    strcpy(result, "0");
    return result;
}

void print_result(char *files[], char *dirs[], char *args, ncmd ncom){
	//printf("args: %s\n", args);
	parse_arguments(args);
	long num_entries;
        int max_nlength;
        char cwd[BUFFSIZE];
        maxLen maxlen, maxtmp;
	long n;

	
        if(ncom.nFiles != 0){
		maxLen maxlen;
		frecord *records = get_frecords(NULL, &maxlen, true, files, &ncom);
		int num_entries = maxlen.lEntries;
		
		if(flag.SORT_ABC) qsort(records, ncom.nFiles, sizeof(frecord), cmp_str);
		
        	for(long nentry = 0; nentry < num_entries; nentry++){
        	
			printf("%-10s %*d %-*s %-*s %*lld %s %-s\n",records[nentry].mode, maxlen.lNlinks, records[nentry].nlinks, maxlen.lUsername, records[nentry].username, maxlen.lGroupname, records[nentry].groupname, maxlen.lSize, records[nentry].size, records[nentry].smtime, records[nentry].name);
        
		}
		if(ncom.nDirs != 0) printf("\n");
        }        
	
	
	qsort(dirs, ncom.nDirs, sizeof(char **), cmp_dirs);
	

	if((flag.SORT_TIME || flag.LAST_ACC || flag.LAST_CHANGE) && !flag.PRINT_LONG){
		if(flag.LAST_MOD){
			qsort(dirs, ncom.nDirs, sizeof(char **), cmp_dir_mtime);
		}else if(flag.LAST_ACC){
			qsort(dirs, ncom.nDirs, sizeof(char **), cmp_dir_atime);
		}else if(flag.LAST_CHANGE){
			qsort(dirs, ncom.nDirs, sizeof(char **), cmp_dir_ctime);
		}
	}

	if(flag.SORT_TIME && flag.PRINT_LONG){
                if(flag.LAST_MOD){
                        qsort(dirs, ncom.nDirs, sizeof(char **), cmp_dir_mtime);
                }else if(flag.LAST_ACC){
                        qsort(dirs, ncom.nDirs, sizeof(char **), cmp_dir_atime);
                }else if(flag.LAST_CHANGE){
                        qsort(dirs, ncom.nDirs, sizeof(char **), cmp_dir_ctime);
                }
        }
	           
        for(int i = 0; i < ncom.nDirs; i++){
                frecord *records, *rectmp;
		maxLen maxlen, mltmp;
		int num_entries;
		char time_string[BUFFSIZE];
		
                if(ncom.nDirs > 1) printf("%s:\n", dirs[i]);
		
		if(flag.HIDDEN_FILES){
        	        records = get_frecords(dirs[i], &maxlen, false, NULL, NULL);
                	num_entries = maxlen.lEntries;
	        }else{
                	rectmp = get_frecords(dirs[i], &mltmp, false, NULL, NULL);

                	records = no_dot_records(rectmp, mltmp.lEntries, &maxlen);
                	num_entries = maxlen.lEntries;
        	}

       		if(flag.SORT_ABC) qsort(records, num_entries, sizeof(frecord), cmp_str);	
                
		if(flag.SORT_TIME){
			if(flag.LAST_MOD) 	qsort(records, num_entries, sizeof(frecord), cmp_mtime);
			if(flag.LAST_ACC)	qsort(records, num_entries, sizeof(frecord), cmp_atime);
			if(flag.LAST_CHANGE)	qsort(records, num_entries, sizeof(frecord), cmp_ctime);
		}
		
		if(flag.SORT_SIZE) qsort(records, num_entries, sizeof(frecord), cmp_size);
		
		if(flag.REVERSE && (flag.SORT_SIZE || flag.SORT_TIME || flag.SORT_ABC)){
			records = reverse_record(records, num_entries);
		}
			
		if(flag.PRINT_LONG && !flag.LONG_UID){
			printf("Total %ld\n", maxlen.lTotal);
        
	        	for(long nentry = 0; nentry < num_entries; nentry++){
				
				int s1 = strcmp(records[nentry].name, ".");
				int s2 = strcmp(records[nentry].name, "..");

				if(flag.UPPER_A && (s1 == 0 || s2 == 0)) continue; 
                		
				if(flag.LAST_MOD){
					strcpy(time_string, records[nentry].smtime);
				}else if(flag.LAST_CHANGE){
					strcpy(time_string, records[nentry].sctime);
                		}else if(flag.LAST_ACC){
					strcpy(time_string, records[nentry].satime);
				}
				char size[BUFFSIZE];
				sprintf(size, "%lld", records[nentry].size);
				
				printf("%-10s %*d %-*s %-*s %*s %s %-s\n",
					records[nentry].mode, maxlen.lNlinks, records[nentry].nlinks, 
					maxlen.lUsername, records[nentry].username, maxlen.lGroupname, 
					records[nentry].groupname, (flag.BYTE_SIZE)?maxlen.lByte:maxlen.lSize, 
					(flag.BYTE_SIZE)?records[nentry].byte:size, 
					time_string, records[nentry].name);
                	}
                	if(i + 1 != ncom.nDirs) printf("\n");
		}

		if(flag.PRINT_LONG && flag.LONG_UID){
                        printf("Total %ld\n", maxlen.lTotal);
        
	                for(long nentry = 0; nentry < num_entries; nentry++){
                                
				int s1 = strcmp(records[nentry].name, ".");
                                int s2 = strcmp(records[nentry].name, "..");

                                if(flag.UPPER_A && (s1 == 0 || s2 == 0)) continue;
				
				if(flag.LAST_MOD){
                                        strcpy(time_string, records[nentry].smtime);
                                }else if(flag.LAST_CHANGE){
                                        strcpy(time_string, records[nentry].sctime);
                                }else if(flag.LAST_ACC){
                                        strcpy(time_string, records[nentry].satime);
                                }
				
				char size[BUFFSIZE];
                                sprintf(size, "%lld", records[nentry].size);
				printf("%-10s %*d %*d %*d %*s %s %-s\n",
					records[nentry].mode, maxlen.lNlinks, records[nentry].nlinks, 
					maxlen.lUid, records[nentry].uid, maxlen.lGid, records[nentry].gid, 
					(flag.BYTE_SIZE)?maxlen.lByte:maxlen.lSize, 
					(flag.BYTE_SIZE)?records[nentry].byte:size, 
					time_string, records[nentry].name);
                        }
                        if(i + 1 != ncom.nDirs) printf("\n");
                }
		if(flag.PRINT_SIMPLE){
			if(flag.UPPER_A){
				frecord *recA;
				recA = malloc(maxlen.lEntries * sizeof(frecord));   
				int f = 0;
				int s1, s2;
				maxLen maxt;
				for(int i = 0; i < maxlen.lEntries; i++){
					
					s1 = strcmp(records[i].name, ".");
                                	s2 = strcmp(records[i].name, "..");

                                	if(flag.UPPER_A && (s1 == 0 || s2 == 0)) continue;
					
					recA[f] =  records[i];
					f++;
				}
				maxt = maxlen;
				maxt.lEntries -= 2;
				if(maxt.lEntries > 0) print_default(recA, &maxt);
			}else{
				if(maxlen.lEntries > 0) print_default(records, &maxlen);
			}
			if(i + 1 != ncom.nDirs) printf("\n");
		}

                chdir("..");
        }

}

void parse_arguments(char *args, ncmd ncom){
	
	int a = 0;
	bool disable_A = false;
	
	while(a < ncom.nArgs){
		
		switch(args[a]){
			
			case 'A' :
				if(!disable_A) flag.UPPER_A = true;
				flag.HIDDEN_FILES = true;
				break;
			case 'a' :
				flag.HIDDEN_FILES = true;
				flag.UPPER_A = false;
				disable_A = true;
				break;
			case 'l' :
				flag.PRINT_LONG = true;
				flag.NAME_SLINK = true;
				flag.LONG_UID = false;
				flag.PRINT_SIMPLE = false;
				break;
			case 'n' :
				flag.PRINT_LONG = true;
				flag.NAME_SLINK = true;
				flag.LONG_UID = true;
				flag.PRINT_SIMPLE = false;
				break;
			case 'f' :
				flag.PRINT_LONG = false;
				flag.NAME_SLINK = false;
				flag.SORT_ABC = false;
				flag.HIDDEN_FILES = true;
				flag.PRINT_SIMPLE = true;
				flag.UPPER_A = false;
				flag.SORT_TIME = false;
				flag.SORT_SIZE = false;
				disable_A = true;
				break;
			case 'u' :
				flag.LAST_ACC = true;
				flag.LAST_MOD = false;
				flag.LAST_CHANGE = false;
				break;
			case 'c' :
				flag.LAST_ACC = false;
				flag.LAST_MOD = false;
				flag.LAST_CHANGE = true;
				break;
			case 't' :
				flag.SORT_TIME = true;
				flag.SORT_ABC = false;
				flag.SORT_SIZE = false;
				break;
			case 'S' :
				flag.SORT_SIZE = true;
				flag.SORT_ABC = false;
				flag.SORT_TIME = false;
				break;
			case 'r' :
				flag.REVERSE = true;
				break;
			case 'h' :
				flag.BYTE_SIZE = true;
				break;
			default : 
				printf("WRONG ARGUMENT\n");
				exit(1);
		}
		a++;
	}
}
        
void print_default(frecord *records, maxLen *maxlen){
	struct winsize w;
        ioctl(0, TIOCGWINSZ, &w);
        int width = w.ws_col;

        int n = maxlen->lEntries;
	int max_name_len = maxlen->lNslname;
        int num_tabs = (max_name_len + TABSIZE -1) / TABSIZE;
        int col_width = num_tabs * TABSIZE;
        int cols = width / col_width;
        int rows = (n + cols - 1) / cols;


        for(int r = 0; r < rows; r++){
                for(int c = 0; c < cols; c++){
			
                        int index = c * rows + r;
                        if(index >= n) continue;
                        //index = id[index];

                        int len = strlen(records[index].nslname);
                        int tabs_needed = (col_width - len + TABSIZE - 1) / TABSIZE;
			
                        printf("%s", records[index].nslname);
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
