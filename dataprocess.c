#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <stdint.h>
#include <math.h>

int argumentParser();
int fileSelector();
int txttocsv();
int csvtotxt();
int writelog();
int printhelp();
char *removeExt();

int main(int argc, char *argv[]) {
	argumentParser(argc,argv);
	remove("temp.log");
	return (EXIT_SUCCESS);
}

int argumentParser(int argc, char *argv[]) {
	if (argc>2){
	char *input = argv[1];
	char *output = argv[2];
	FILE *opfp = fopen(argv[2], "r");
	FILE *tmplog = fopen("templog","w");
	if (opfp!=NULL){
	char c;
	fclose(opfp);
	printf("Warning: %s already exists. Do you wish to overwrite (y,n)?",
	argv[2]);
	scanf("%c", &c);
	if (c=='y'){
	FILE *opfp = fopen(argv[2], "w");
	if (opfp==NULL){
	printf("Error 02: %s is not allowed to be overwrited", argv[2]);
	fprintf(tmplog,"Error 02: %s is not allowed to be overwrited", argv[2]);
	fclose(tmplog);
	return 1;
	}
	fclose(opfp);
	}
	else{
		printf("Exiting without processing");
		fprintf(tmplog,"Exiting without processing");
		fclose(tmplog);
		return 1;
		}
	}
	fclose(tmplog);
	fileSelector(input,output);
	writelog(input,output);
	}
	else if (strcmp(argv[1],"-m")==0)
		printhelp();
	else printf("Error 10 : Unknown Command\nUse \"dataprocess -m\" to see the help");
	return (EXIT_SUCCESS);
}

int fileSelector(char *input, char *output) {
	char *ext;
	ext = strchr(input, '.');
	if (strcmp(ext,".txt")==0){
		txttocsv(input,output);
	}
	else if (strcmp(ext,".csv")==0){
		csvtotxt(input,output);
	}
	return (EXIT_SUCCESS);
}	

int csvtotxt(char *input, char *output) {
	FILE *csv = fopen(input, "r");
	FILE *txt = fopen(output, "w");
	FILE *tmplog = fopen("templog","w");
	if (!csv){
		printf("Error 01 : %s could not be opened" , input);
		fprintf(tmplog,"Error 01 : %s could not be opened" , input);
		return 1;
	}
	struct stat sb;
	stat(input, &sb);
	char *header = malloc(sb.st_size);	
	if (fscanf(csv, "%[^\n] ", header) == EOF){      // read header 
		printf("Error 02 : %s is empty", input);
		fprintf(tmplog,"Error 02 : %s is empty", input);
	}
	int id;
	char time[19];
	float temp;
	int humdt;
	int tmpid;
	char tmptime[19];
	float tmptemp;
	int tmphumdt;
	int lineno = 1;
	char buf[35];
	int len;
	
	while (fgets(buf, sizeof buf, csv)) {
		++lineno;
		if(sscanf(buf, "%d,%[^,],%f,%d", &tmpid, tmptime, &tmptemp, &tmphumdt) == 4){
			id = tmpid;
			strcpy(time, tmptime);
			temp = tmptemp;
			humdt = tmphumdt;
			len = 10;
		}
		else if (sscanf(buf, "%d,%[^,],%f,", &tmpid, tmptime, &tmptemp) == 3){
			id = tmpid;
			strcpy(time, tmptime);
			temp = tmptemp;
			humdt = -1;
			len = 10;
		}
		else if(sscanf(buf, "%d,%[^,],,%d", &tmpid, tmptime, &tmphumdt) == 3){
			id = tmpid;
			strcpy(time, tmptime);
			temp = -11;
			humdt = tmphumdt;
			len = 10;
		}
		else {
			printf("Error 11 : Invalid data in row %d.Possibly missing values\n", lineno);
			fprintf(tmplog,"Error 11 : Invalid data in row %d.Possibly missing values\n", lineno);
			continue;
		}
		uint32_t u;
		memcpy(&u, &temp, sizeof u);
		int b4 = u & 0xff;
		int b3 = (u >> 8) & 0xff;
		int b2 = (u >> 16) & 0xff;
		int b1 = (u >> 24) & 0xff;
		struct tm tm;
		time_t sec;
		if (sscanf(time, "%d-%d-%d %d:%d:%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec) != 6) {
			printf("Error 12 : Error time format");
			fprintf(tmplog,"Error 12 : Error time format");
			return 1;
	}
		tm.tm_year -= 1900;
		tm.tm_mon -= 1;
		tm.tm_hour -= 1;
		tm.tm_min -= 30; // correcting 1 hour 30 min error
		sec = mktime(&tm) ;
		uint32_t seconds = mktime(&tm); 
		int s4 = seconds & 0xff;
		int s3 = (seconds >> 8) & 0xff;
		int s2 = (seconds >> 16) & 0xff;
		int s1 = (seconds >> 24) & 0xff;
		if (temp >= -10.0 && temp <= 51.0 ) len+=4;
		fprintf(txt,"%02x %02x ", 0 & 0xff,len & 0xff);
		fprintf(txt,"%02x ", id & 0xff);
		fprintf(txt,"%02x %02x %02x %02x " , s1,s2,s3,s4);
		if (temp >= -10.0 && temp <= 51.0 ) fprintf(txt,"%02x %02x %02x %02x " , b1,b2,b3,b4);
		else {
			printf("Error 13: Temparature is out of range or missing\n");
			fprintf(tmplog,"Error 13: Temparature is out of range or missing\n");
		}
		if (humdt >= 40 && humdt <= 95) fprintf(txt,"%x " , humdt & 0xff);
		else {
			printf("Error 14: Humidity is out of range or missing\n");
			fprintf(tmplog,"Error 14: Humidity is out of range or missing\n");
		}
		int sum = len + id + seconds + u + humdt;
		int checksum = (~sum)+1;
		fprintf(txt,"%02x ", checksum & 0xff);
		fprintf(txt,"%02x\n", 255 & 0xff);
	}
	fclose(csv);
	fclose(txt);	
	fclose(tmplog);
}


int txttocsv(char *input, char *output) {
	FILE *txt = fopen(input, "r");
	FILE *csv = fopen(output, "w");
	FILE *tmplog = fopen("templog","w");
	if (!txt){
		printf("Error 01 : %s could not be opened" , input);
		fprintf(tmplog,"Error 01 : %s could not be opened" , input);
		return 1;
	}
	struct stat sb;
	stat(input, &sb);
	char *line = malloc(sb.st_size);	
	int len = 0;
	int b1,b2,b3,b4,s1,s2,s3,s4,id,humdt;
	while (fscanf(txt, "%[^\n] ", line) != EOF){      
		if(sscanf(line, "00 %x %x %x %x %x %x %[^\n] ",&len,&id,&s1,&s2,&s3,&s4,line) == 7) {
			uint32_t seconds = (s1 << 24) | (s2 << 16) | (s3 << 8) | s4;
			static char time[128];
			struct tm *gmt;
			seconds+= 0x00001518; // correcting 1 hour 30 min error
			time_t tmp = (time_t)seconds;
			gmt = localtime(&tmp);
			strftime (time, sizeof(time), "%Y-%m-%d %H:%M:%S", gmt);
		        fprintf(csv,"%d,%s,", id,time);	
			if (len == 14){
				if(sscanf(line, "%x %x %x %x %x %[^\n]", &b1,&b2,&b3,&b4,&humdt,line) == 6) {
			uint32_t raw = (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
			union { uint32_t b; float f; } u;
			u.b = raw;
			fprintf(csv,"%.2f,%d\n" , u.f , humdt);
				}

			}

		}	
	}
	fclose(tmplog);
	fclose(csv);
	fclose(txt);	

}

int writelog(char *input, char *output){
	time_t now = time(NULL);
	struct tm tm = *localtime(&now);
	FILE *tmplog = fopen("templog","r");
	if (!tmplog) return 1;
	char filename[100];
	char *newip=removeExt(input);
	char *newop=removeExt(output);
	sprintf(filename,"%s_%s_%d%d%02d_%d%d%d.log",newip,newop,tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);
	FILE *log = fopen(filename, "w");
	struct stat sb;                     
	stat("templog", &sb);             
        char *line = malloc(sb.st_size);    
        while(fscanf(tmplog, "%[^\n] ", line) != EOF){
             fprintf(log, "%s\n", line);
       }
       fclose(tmplog);
       fclose(log);
}
	
char *removeExt(char *filename){
	size_t len = strlen(filename);
	char *newfilename = malloc(len);
	for(int i = len-1; i>=0 ; i--){
		if(filename[i]=='.'){
			memcpy(newfilename,filename,i);
			newfilename[i]=0;
			break;
		}
		else
			newfilename=filename;
	}
	return newfilename;
}

int printhelp(){
	char *helpmsg = "Usage: dataprocess [INPUTFILE] [OUTPUTFILE] \nConvert sensor data in text format to binary packet and viceversa.\n\tdataprocess -m\t\t-\tprint this help message";
	printf("%s",helpmsg);
}

