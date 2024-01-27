#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

int
main(int argc, char *argv[])
{
    int fds[2];
    
    if (pipe(fds) == -1) {
        printf("Pipe ERROR\n");
        return 0;
    }
    
    int commfds[2];
    
    if (pipe(commfds) == -1) {
        printf("Pipe ERROR\n");
        return 0;
    }

    printf("I’m SHELL process, with PID: %d - Main command is: man grep | grep -A 4 -n  \"-r, --recursive\" > output.txt\n", (int) getpid());
    int first_fork = fork();
    
    
    if(first_fork < 0){
    	fprintf(stderr, "fork failed\n");
        exit(1);
    }else if(first_fork==0){
    	//MAN PROCESS
    
    	close(commfds[0]);
    	
    
    	//Child of the shell process
    	printf("I’m MAN process, with PID: %d - My command is: man grep\n",(int)getpid());
    	
    	
    	//the message is not important actually. It is just for MAN to inform GREP that it has passed printing operation.
    	char message[] = "I HAVE PRINTED OUT MY MESSAGE GREP, YOU CAN ALSO PRINT YOURS!\n";
        write(commfds[1], message, strlen(message));

    	close(commfds[1]);
    	
    	//we are not going to read anything, rather we need to write the output of man grap so that we can read in GREP process
    	close(fds[0]);
    	
    	dup2(fds[1], STDOUT_FILENO);
  
    	
    
    	char *exec_args[3];
    	exec_args[0]="man";
    	exec_args[1]="grep";
    	exec_args[2]=NULL;
    	
    	execvp(exec_args[0],exec_args);
    	
    	
    }else{
    	//SHELL PROCESS
    	
    
    	
	int second_fork=fork();
	    
	if(second_fork<0){
		fprintf(stderr, "fork failed\n");
        	exit(1);
	}else if(second_fork==0){
	
		//GREP PROCESS
	
		close(commfds[1]);
		
		char buffer[BUFSIZ];
		ssize_t bytesRead;
		
		// read is blocking, so it waits here once it finds out that MAN has printed
		bytesRead = read(commfds[0], buffer, BUFSIZ);
			
		printf("I’m GREP process, with PID: %d - My command is: grep -A 4 -n  \"-r, --recursive\" > output.txt\n",(int)getpid());
		
		
		
		
		
		close(commfds[0]);
	
	
		//we're not going to write but read
		close(fds[1]);
		
		
		//in order to get the input from read end            
       		dup2(fds[0], STDIN_FILENO);
       		
       		
       		int fileNumber=open("output.txt", O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
       		
       		//in order to write to output to file named output.txt
       		dup2(fileNumber,STDOUT_FILENO);
       		
       		char *exec_args[3];
    		exec_args[0]="grep";
    		exec_args[1]="-A";
    		exec_args[2]="4";
    		exec_args[3]="-n";
    		exec_args[4]="\\-r, --recursive";
    		exec_args[5]=NULL;
    	
    		execvp(exec_args[0], exec_args);
    		
	}else{
	
		//SHELL PROCESS
		
		
		close(commfds[0]);
		close(commfds[1]);
		close(fds[0]);
		close(fds[1]);
		//we need to wait for GREP&MAN process
		int status1;
		int status2;
		waitpid(second_fork, &status1, 0);
		waitpid(first_fork,&status2,0);
		fprintf(stdout,"I’m SHELL process, with PID: %d - execution is completed, you can find the results in output.txt\n", (int) getpid());
		
	
	}
	 
	  
    
    
    }
    
    
    return 0;
}
