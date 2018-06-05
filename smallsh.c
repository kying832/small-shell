/*
	Author: Kevin Ying
	OSU CS 344 - Project 3 smallsh
	ID: 933249073

  */

//BIG TODO-S  
//get input/output redirection up and running  ---THIS IS NOW COMPLETE!!!---
//handle background/foreground processes and modes ---THIS IS ALMOST COMPLETE --TODO HANDLE BG/FG MODE!---
//signal handling for SIGINT foreground vs shell  ---THIS IS NOW COMPLETE!!!---
//SIGINT shell itself will ignore a SIGINT, but any foreground process will be terminated.
//SIGTSTP shell cannot run anything else new in background as a switch i.e background processes on/off
//smaller, but still important TODO - convert any instance of $$ to process ID of shell in getline string or strtok array ---this is done, but there appears to be a bug w/ the grading script, inserting @'s
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
volatile sig_atomic_t bg_on_flag = 1;

void catchSIGINT(int signo)
{
	write(2, "sigint caught\n", 14); 
	write(2, "Process terminated by signal: ", 30);
//	int termSig =  WTERMSIG(signo);
//	printf("%d\n", signo);//not reentrant, this is temporary

	write(2, (char*)&signo, sizeof(int));
//	write(2, &signo, 4);
	return;	
}

void catchSIGTSTP(int signo)
{
	if(bg_on_flag == 1)
	{
		bg_on_flag = 0;
		write(1, "Background Processes Disabled.\n", 31);
		return;
	}
	if(bg_on_flag == 0)
	{
		bg_on_flag = 1;
		write(1, "Background Processes Enabled.\n", 31);
		return;
	}

}
	//this will control background vs foreground process
	//value of 1 means BG functionality is enabled
	//value of 0 means BG functionality is disable, FG only

//this function will take in the raw getline string and parse it for the characters $$
//then, it will get the process ID using getpid()
//
//TODO FREE MEMORY FROM W/e IS returned?
char * expand$$(char * str)
{
	size_t len = strlen(str);
	pid_t procID = getpid();
	char * newStr = (char *) malloc(50 * sizeof(char));
	char tempStr[strlen(str)];
	memset(tempStr, '\0', strlen(tempStr));
	memset(newStr, '\0',  strlen(newStr));
//	printf("pid: %d\n", procID);
	//find the incidence of $$
	int i;
	for(i = 0; i < len; i++)
	{
//		printf("%c\n", str[i]);
		if(str[i] == '$' && str[i + 1] == '$')
		{	
			memcpy(tempStr, str, i);
//			printf("the temp string is: %s\n", tempStr);
			sprintf(newStr, "%s%d", tempStr, procID);
//			printf("the new string is: %s\n", newStr);
			return newStr;
		}
	}	
	return str;
}
	

int main()
{
	//set up signal handlers
	//for SIGINT, the signal will proceed uninterrupted and cut off the child exec process
	//this is default behavior, so only parent need to do something.
	//in the parent process, the signal will be captured and output to the terminal.
	struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0}, ignore_sig = {0};
	//setup SIGINT capture
	SIGINT_action.sa_handler = catchSIGINT;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = 0;
//	sigaction(SIGINT, &SIGINT_action, NULL);
//setup SIGTSTP capture
	SIGTSTP_action.sa_handler = catchSIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
//	SIGTSTP_action.sa_flags = SA_RESTART;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);
	//setup signal ignore
	ignore_sig.sa_handler = SIG_IGN;
	sigaction(SIGINT, &ignore_sig, NULL);
		//all built in functions are handled by above?
		//next, set up fork/exec functionality
	pid_t bg_array[10];
		int bg_proc_count = 0;
	
	char * buffer;
	char * token;
	char * tokArr[512];
	int tokenCount;
	char * space_delim = " ";
	size_t  size = 2048;
	size_t inputsize;
	buffer = (char *) malloc(size *sizeof(char));
	pid_t childPID;
	int childExitMethod = -5; //set method to -5.  if unchanged, have an if that will check if nothing's changed and invalidate
	int bgExitMethod = -5;
	int bg_run_flag = -1;	//this will control background vs foreground process in MAIN
	//value of 1 means input string has an & at the end, run in BG
	//value of 0 means inpuut string has no &, run in FG
	//should be sufficient to check if bg_on_flag is on.  If yes, check last token for ampersand.  If yes, set bg_run_flag to 1
	//if bg_on_flag is set to 0, ignore any ampersands maybe by NULLing the token and run in FG
	//if bg_on_flag is set to 1, and no trailing &, then run in FG
//	memset(buffer, '\0', sizeof(buffer));
	//this loop constitues core of the shell
	while(1)
	{
		//background wait checking here w/ WNOHANG
		pid_t bg_PID = waitpid(-1, &bgExitMethod, WNOHANG);
		int m;
		for(m = 0; m < bg_proc_count; m++)
		{
			if(bg_PID == bg_array[m])
			{
				//if the bg PID matches something in the array
				//notify
				if(WIFEXITED(bgExitMethod))
				{
				printf("Process ID %d has exited with value %d\n", bg_PID, WEXITSTATUS(bgExitMethod));
				fflush(stdout);
				}
				if(WIFSIGNALED(bgExitMethod))
				{
				printf("Process ID %d has received signal %d\n", bg_PID, WTERMSIG(bgExitMethod));
				fflush(stdout);
				}

			}
		}
	
		tokenCount = 0;
		write(1, ": ", 2);
	while(1)
	{
	
		inputsize = getline(&buffer, &size, stdin);	//getline may leave behind memory leak, TODO
		if(inputsize == -1)
			clearerr(stdin);
		else 
			break;
	}
		if(buffer[0] == '\n' || buffer[0] == '#') continue; //skip newlines and comments
		//handle input instances
		//chop off newline at end of string
		if(buffer[strlen(buffer) - 1] == '\n')
			buffer[strlen(buffer)-1] = '\0';
//		printf("buffer before expansion: %s\n", buffer);
		buffer = expand$$(buffer);
//		printf("new buffer after expansion is: %s\n", buffer);
	
/*	
		int var;
		for(var = 0; var < strlen(buffer); var++)
		{
		//	printf("do i segout before i access?\n");
		//	printf("%d %s \t %d\n",var, buffer[var], buffer[var]);
			printf("%d  %c   %d\n", var, buffer[var], buffer[var]);
		}

*/			
		token = strtok(buffer, space_delim);
		//string processing block
		//COMPLETE -  process < and > i/o redirection symbols
		//this loop writes all string tokens into the token array tokArr
		while(token != NULL)
		{
			tokArr[tokenCount] = token;
			tokenCount++;
			token = strtok(NULL, " ");

		}
		//set the last ptr in tokArr to NULL for use w/ execvp
//		printf("token count is = %d", tokenCount);
			tokArr[tokenCount] = NULL;

		//now check for presence of & as last token to prep for bg vs fg	
		if(bg_on_flag == 1) //if bg procs are enabled
		{
			if(strcmp(tokArr[tokenCount - 1], "&") == 0)
			{
			//check for ampersand in last token to set the bg_fun_flag to 0 or 1
				bg_run_flag = 1;
				//and wipe out the ampersand so it doesn't pollute  the exec command later
				tokArr[tokenCount - 1] = NULL;
				tokenCount--;
			//	printf("running in background!\n");
			//	fflush(stdout);
			}
			else
			{
				bg_run_flag  = 0;
			//	printf("running in foreground!\n");
			//	fflush(stdout);
			}
		}
		else	//if bg procs are disabled
		{
			bg_run_flag = 0;
			if(strcmp(tokArr[tokenCount-1], "&") == 0)
			{
		//	printf("%s\n", tokArr[tokenCount-1]);
			//and wipe out the ampersand so it doesn't pollute  the exec command later
			tokArr[tokenCount - 1] = NULL;
			tokenCount--;
			}
		}
		//setup i/o redirection if needed
		int a;
		int file_descrip_r, file_descrip_w;
		int read_flag = 0 , read_ind = -5;
		int write_flag = 0, write_ind = -6;
		for(a = 0; a < tokenCount; a++)
		{
			if(strcmp(tokArr[a], "<") == 0)
			{
				//set read file flag
				read_flag = 1;
				//store index of file name
				read_ind = a + 1;			
				//NULL past the command
				//i.e.
				//a[0] = echo
				//a[1] = hello
				//a[2] = >
				//a[3] = place
				//becomes
				//a[0] = echo
				//a[1] = hello
				//a[2] = NULL
				//rest of array technically present, but should have no impact on execvp
				//
		//		tokArr[a] = NULL;
			}
			if(strcmp(tokArr[a], ">") == 0)
			{
				//set write file flag
				write_flag = 1;
				//store index of file name
				write_ind = a + 1;

		//		tokArr[a] = NULL;
			}
		}
		
//		int res1, res2;
		//process read/write flags and create file descriptor based on those flags
		//case 1: read only
		if(read_flag == 1 && write_flag != 1)
		{

			file_descrip_r = open(tokArr[read_ind], O_RDONLY);
			if(file_descrip_r == -1)
			{
				perror("open file failed\n");
			//	exit(3);
				continue;
			}
			//			res1 = dup2(file_descrip_r, 0);
//			if(res1 == -1)
//			{
//				perror("dup2 failed\n");
//				exit(2);
//			}

		}
		//case 2: write only	
		if(read_flag != 1 && write_flag == 1)
		{
			//printf("file name is: %s\n", tokArr[write_ind]);

			file_descrip_w = open(tokArr[write_ind], O_CREAT|O_WRONLY, 0644);
			if(file_descrip_w == -1) 
			{
				perror("open file failed\n");
				continue;
				//exit(3);
			}
//			res2 = dup2(file_descrip_w, 1);
//			if(res2 == -1)
//			{
//				perror("dup2 failed\n");
//				exit(2);
//			}
		}
		//case 3: read and write both
		if(read_flag == 1 && write_flag == 1)
		{
			file_descrip_r = open(tokArr[read_ind], O_RDONLY);
			file_descrip_w = open(tokArr[write_ind], O_CREAT|O_WRONLY, 0644);
			if(file_descrip_r == -1 || file_descrip_w == -1)
			{
				perror("open file failed\n");
				continue;
			}
//			res1 = dup2(file_descrip_r, 0);
//
//			res2 = dup2(file_descrip_w, 1);
//			if(res1 == -1 && res2 == -1 )
//			{
//				perror("dup2 failed\n");
//				exit(2);
//			}
		}
/*		int j;
		for(j = 0; j  < tokenCount; j++)
		{
			printf("%s\n", tokArr[j]);
		}
*/
		//exit handler
		if(strcmp(tokArr[0], "exit") ==0)
		{
			token = strtok(NULL, space_delim);
			if(tokArr[1] == NULL)
			{
				//TODO HANDLE CHILD PROCESS TERMINATION HERE
				 exit(0);
			}
			else 
			{
				printf("invalid command\n");
				continue;
			}	
			
		}//end exit handler
		//cd handler
		//TODO CHECK IF CD HANDLER ACTUALLY CHANGES DIRECTORY WITH LS FUNCTIONALITY LATER
		if(strcmp(tokArr[0], "cd") == 0)
		{
		//	token = strtok(NULL, space_delim);
			if(tokArr[1] == NULL)
			{
				//change directory to the home directory
				//taken from https://stackoverflow.com/questions/9493234/chdir-to-home-directory?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa
				chdir(getenv("HOME"));
				continue;
			}
			else
			{
				//change directory to the specified directory
				chdir(tokArr[1]);
				continue;
			}

		}//end cd handler
		//status handler
		//TODO STATUS HANDLER.  PROBABLY NEED TO DEFINE PROCESS ID'S FIRST
		//this is actually done, need to verify w/ sleep and signal handling, etc...
		//may also need to be relocated?
		if(strcmp(tokArr[0], "status") == 0)
		{
		//	printf("Status of last exited status: %d\n", childExitMethod);
			//actually need to parse childExitMethod using
			//WIFEXITED and WIFSIGNALLED macros
			//if WIFEXITED is set then run WEXITSTATUS to get the status
			//if WIFSIGNALLED is set, then run WTERMSIG to get the signal that killed it
			if(childExitMethod == -5)
			{
				//this block, no child's been created, so say so?
				printf("Invalid: No status set yet.\n");
				fflush(stdout);
				continue;
			}
			if(WIFEXITED(childExitMethod) != 0 && childExitMethod != -15)
			{
				int exitStatus = WEXITSTATUS(childExitMethod);
				printf("Exit status: %d\n", exitStatus);
				fflush(stdout);
				continue;
			}
			if(WIFSIGNALED(childExitMethod) != 0 && childExitMethod != -15 ) 
			{
				int termSig =  WTERMSIG(childExitMethod);
				printf("Terminated by signal: %d\n", termSig);
				fflush(stdout);
				continue;
			}
		}
		childPID = fork();
	if(bg_run_flag == 0)
	{
		switch(childPID){
			case -1: 
			{
				perror("forking has gone horribly wrong!\n");
				exit(1);
				break;
			}
			case 0:
			{
				//child process will ignore SIGTSTP
				sigaction(SIGTSTP, &ignore_sig, NULL);
				//this is the child process 
				//here, exec will be called with appropriate handlers in time
				//TODO write signal handlers
				struct sigaction SIGINT_child = {0} ;
				//setup SIGINT capture in child process
				SIGINT_child.sa_handler = SIG_DFL;
				sigfillset(&SIGINT_child.sa_mask);
				SIGINT_child.sa_flags = 0;
				sigaction(SIGINT, &SIGINT_child, NULL);


				//dealing w/ i/o redirection
				int res1, res2;
				if(read_flag == 1 && write_flag != 1)
				{
					res1 = dup2(file_descrip_r, 0);
					if(res1 == -1)
					{
						perror("dup2 failure\n");
						exit(2);
					}
				}
				if(read_flag != 1 && write_flag == 1)
				{
					res2 = dup2(file_descrip_w, 1);
					if(res2 == -1)
					{
						perror("dup2 failure\n");
						exit(2);
					}

				}
				if(read_flag == 1 && write_flag == 1)
				{
					res1 = dup2(file_descrip_r, 0);
					res2 = dup2(file_descrip_w, 1);
					if(res1 == -1 || res2 == -1)
					{
						perror("dup2 failure\n");
						exit(2);
					}
				}
				//for now, let's do the simple case, and make more complex later
				
				//let's check what's stored in tokArr before execvp is called
				//printf("token array contents:\n");
				int b = 0;
				for(b; b < tokenCount; b++)
				{
					if(*(tokArr[b]) == '<' ||*( tokArr[b]) == '>')
					{
					//	fprintf(stderr, "NULL  HERE");
						tokArr[b] = NULL;
						continue;
					}
				//	fprintf(stderr,"%s\n", tokArr[b]);
				}
				
	//			printf("Child is about to EXEC out!\n");		
	//			fflush(stdout);
				execvp(tokArr[0],tokArr);
				perror("exec fail!\n");
				exit(1);
				break;
			}
			default:
			{
				//this is the parent process
				//store childExitMethod in this block, it'll be accessible 
				//in next loop iteration

				//simple wait
				//sleep(5);	//experiment - does a wait while the process is terminated ensure exit method is filled?
				pid_t actual;
				do{
				 	actual = waitpid(childPID, &childExitMethod, 0);
		
				}while(actual != childPID);       
	//			= waitpid(childPID, &childExitMethod, 0);
			//	pid_t actual = wait(&childExitMethod);
			//	printf("back in parent!\n");
			//	fflush(stdout);
				
			//	if(actual == childPID)
			//	{
				
				
				//	printf("exit method = %d\n", childExitMethod);
				//	printf("signal status = %d\n", WIFSIGNALED(childExitMethod));
				//	fflush(stdout);
				//	printf("signal code: %d\n", WTERMSIG(childExitMethod));
					if((WIFSIGNALED(childExitMethod)) &&( childExitMethod != -5))
					{
						int termSig =  WTERMSIG(childExitMethod);
						printf("Terminated by signal: %d\n", termSig);
						fflush(stdout);
						continue;
					}
			//	}
				break;
			}

		}//end switch for fork execution
	}//end  foreground execution block
	else //start background execution block
	{
//		childPID =  fork();
	//open /dev/null/ for bg process read/write if no file is specified
		//can reuse earlier < / > flags 
		int dev_null = open("/dev/null", O_RDWR);
		if(dev_null == -1)
		{
			perror("open has gone horribly wrong.\n");
			exit(2);
		}
		//storage array for ongoing BG procs
	switch(childPID)
		{
			case -1:
			{
				perror("forking has gone horribly wrong\n");
				exit(1);
				break;
			}
			case 0:
			{
				//child process will ignore SIGTSTP
				sigaction(SIGTSTP, &ignore_sig, NULL);
				//background child block
				int res1, res2;
				//if input file specified, but no output specified
				if(read_flag == 1 && write_flag != 1)
				{
					res1 = dup2(file_descrip_r, 0);
					res2 = dup2(dev_null, 1);
					if(res1 == -1 || res2 == -1)
					{
						perror("dup2 failure\n");
						exit(2);
					}
				}
				//only output file specified, route input to dev/null
				if(read_flag != 1 && write_flag == 1)
				{
					res2 = dup2(file_descrip_w, 1);
					res1 = dup2(dev_null, 0);
					if(res2 == -1 || res1 == -1)
					{
						perror("dup2 failure\n");
						exit(2);
					}

				}
				//both input and output specified, do just like  FG case
				if(read_flag == 1 && write_flag == 1)
				{
					res1 = dup2(file_descrip_r, 0);
					res2 = dup2(file_descrip_w, 1);
					if(res1 == -1 || res2 == -1)
					{
						perror("dup2 failure\n");
						exit(2);
					}
				}
				int b = 0;
				for(b; b < tokenCount; b++)
				{
					if(*(tokArr[b]) == '<' ||*( tokArr[b]) == '>')
					{
					//	fprintf(stderr, "NULL  HERE");
						tokArr[b] = NULL;
						continue;
					}
				//	fprintf(stderr,"%s\n", tokArr[b]);
				}
				
	//			printf("Child is about to EXEC out!\n");		
	//			fflush(stdout);
				execvp(tokArr[0],tokArr);
				perror("exec fail!\n");
				exit(1);
				break;


			}//end child process handling
			default:
			{
				//background parent block
				//these are going to contain the bg process id's
			/*	pid_t bg_array[10];
				int bg_proc_count = 0;
				
			*/
				//add last process to bg array
				int status;
				bg_array[bg_proc_count] = childPID;
				bg_proc_count++;
				printf("Background Process ID: %d\n", childPID);
				break;	
			}
		}//end switch block

	}//background execution block


		/*		
 *	if((WIFSIGNALED(childExitMethod) != 0 ) && childExitMethod != -15)
			{
				int termSig =  WTERMSIG(childExitMethod);
				printf("Terminated by signal: %d\n", termSig);
				fflush(stdout);
			}
*/					
	//	printf("buffer contents: %s\n", buffer);

	}//end big while loop
	


	return 0; //end main
}
