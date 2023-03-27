#include <stdio.h>
#include <string.h>
#include <syslog.h>

int main( int argc, char** argv )
{
	char *writefile;
	char *writestr;
	int  status = 0;

	openlog( "Assignment2", 0, LOG_USER );

	if( argc != 3 )
		{
		printf("Warning!!  Function requires 2 parameters.  [writer writefile writestr]\n");
		syslog(LOG_ERR, "Incorrect number of input parameters");
		status = 1;
		}
	else
	    	{
		writefile = argv[1];
		writestr  = argv[2];
		}

	if( status == 0 )
		{
		FILE *file1 = fopen( writefile, "w" );
		if( file1 == NULL )
			{
			printf("Warning!!  Cannot access the file %s\n", writefile);
			syslog(LOG_ERR, "File write error with %s", writefile);
			status = 1;
			}
		else
			{
			fwrite( writestr, strlen(writestr), 1, file1 );
			syslog(LOG_DEBUG, "Success writing %s into %s", writestr, writefile);
			fclose( file1 );
			}
		}
	return status;
}
