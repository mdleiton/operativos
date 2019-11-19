//Fuente https://stackoverflow.com/questions/20013693/read-csv-file-to-a-2d-array-on-c

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main()
{
   char buffer[1024] ;
   char *record,*line;
   int i=0,j=0,a,b;
   int mat[14][14];
   FILE *fstream = fopen("matrizDistancia.csv","r");
   if(fstream == NULL)
   {
      printf("\n file opening failed ");
      return -1 ;
   }
   while((line=fgets(buffer,sizeof(buffer),fstream))!=NULL)
   { j=0;
     record = strtok(line,",");
     while(record != NULL)
     {
    // printf("record : %s",record) ;    //here you can put the record into the array as per your
   
     mat[i][j] = atoi(record) ;
     record = strtok(NULL,",");
     j++;	
     }     
     i++ ;
   }

   for (a=0;a<i;a++){	
	   for (b=0;b<j;b++)	
		{printf("-%d-",mat[a][b]);}
   	printf("\n");
   }

   return 0 ;
 }
