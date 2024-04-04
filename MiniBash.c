#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/wait.h>
#include<string.h>

// to hold the background process id
int b_pid[1024];
// to hold the background process count
int b_count=0;

// concate the file content and display it to standard output
// take two argument file_args array
// file count;
void  file_concat(char *file_args[],int file_count){
	// iterate over the file args array
	for(int i=0;i<file_count;i++){
		// remove blank space
		char *args = strtok(file_args[i]," ");
		// open file in read only mode
		int fd = open(args,O_RDONLY);
		if(fd==-1){
			printf("Error while opening a file in read only mode \n");
			continue;
		}
		// read 1 byte buffer array
		char buff[1];
		int n;

		// read file content one by one byte 
		while((n=read(fd,buff,sizeof(buff)))>0){
			// write file content to standard output.
			if(write(1,buff,1)==-1)
				continue;
		}
	}
}


// method to execute command entered by the user using execvp

int run_process(char *args[]){
	// create a child process
	int chld = fork();
	if(chld==-1){
		// error while forking
		exit(-1);
	}else if(chld==0){
		// child code execution
		int e = execvp(args[0],args);
		if(e==-1){
			exit(-1);
		}
		exit(0);
	}else {
		// parent code execution
		int status;
		// wait for child to complete
		waitpid(chld,&status,0);
		// check for child processed successfully or not.
		if(WIFEXITED(status) && WEXITSTATUS(status)==0){
			return 0;
		}
		return -1;
	}
}

// execute pipe command
// take three arguments
// 1. array of character pointers
// 2. input file descriptor
// 3. output file descriptor
int run_pipe(char *args[], int i_fd,int o_fd){
	// create a child process
	int chld = fork();
	if(chld==-1){
		printf("Error while forking\n");
		exit(-1);
	}else if(chld==0){
		// child process
		//  check input file descriptor is equal to standard input or not.
		if(i_fd!=0){
			//dup2 refer standard input to i_fd
			int d=dup2(i_fd,0);
			if(d==-1){
				exit(-1);
			}else{
				close(i_fd);
			}
		}

		if(o_fd!=1){
			// dup2 refer standard output to o_fd
			int d=dup2(o_fd,1);
			if(d==-1){
				exit(-1);
			}else{
				close(o_fd);
			}
		}
		// execute command
		int e = execvp(args[0],args);
		if(e==-1){
			exit(-1);
		}
		return 0;
	}else {
		int status;
		// wait for a child process to complete
		waitpid(chld, &status,0);
		if(WIFEXITED(status) && WEXITSTATUS(status)==0){
			return 0;
		}
		return -1;
	}

}


// run process in background
// take two argumentd
// 1. command array
// 2. number of command
void run_background(char *cmds[], int cmd_count){
	// replace & to NULL and decrease the count by 1
	if(strcmp(cmds[cmd_count],"&")==0){
		cmds[cmd_count-1]=NULL;
		cmd_count--;
	}
	// create a child process
	int chld = fork();
	// error while forking
	if(chld==-1){
		exit(-1);
	}else if(chld==0){
		// child process execution 
		if(execvp(cmds[0],cmds)==-1){
			exit(-1);
		}
	}else{
		// both child and parent process run concurrently 
		// store child in global array to get it back in foregound
		b_pid[b_count++]=chld;
		printf("[%d][%d]\n",getpid(),getppid());
	}

}

// this method for preparing argument array and check validation for argument count
void prepare_args(char *cmd,char *args[6],int *argc){
	char *arg = strtok(cmd," ");
	while(arg != NULL){
		// if it is more than 4 than throgh an error by saying that "Too many arguments"
		if(*argc>4){
			printf("Too many arguments !!\n");
			exit(-1);
			return;
		}
		args[(*argc)++]=arg;
		arg = strtok(NULL," ");
	}
}

// method to execute commnad one by one sequentially separated by ;
void run_sequence(char *cmds[], int cmd_count){
	// iterate through each commnad
	for(int i=0;i<cmd_count;i++){
		char *args[6];
		int argc=0;
		// prepare arguments array for each command
		prepare_args(cmds[i],args,&argc);
		args[argc]=NULL;
		run_process(args);
	}
}

// this method is used for to execute command seperated by pipe
void run_pipes(char *cmds[], int cmd_count){
	int  i_fd=0;
	// create a pipe array
	int ar_pipe[2];
	// iterate throgh each command in pipe
	for(int i=0;i<cmd_count;i++){
		char *args[6];
		int argc=0;
		// prepare argument for each command
		prepare_args(cmds[i],args,&argc);
		args[argc]=NULL;
		// check for it is the last commnad in pipe or not
		// if yes = pass o_fd as 1 to display output to standard output
		if(i==cmd_count-1){
			run_pipe(args,i_fd,1);
		}else{
			// otherwise create pipe for read end and write end
			if(pipe(ar_pipe)==-1){
				exit(-1);
			}
			//call run_pipe with i_fd and pipe write end
			run_pipe(args,i_fd, ar_pipe[1]);
			close(ar_pipe[1]);
			// modify standard input to pipe read end
			i_fd=ar_pipe[0];
		}
	}

}

// This method is used for to execute input redirection command
void run_ipredirection(char *ip[],int index,int type){
	char *ip_file = ip[index+1];
	ip[index]=NULL;
	// open file in read only mode
	int fd= open(ip_file,O_RDONLY);
	if(fd==-1){
		exit(-1);
	}
	// dup2 refer standard input to file
	if(dup2(fd,0)==-1){
		exit(-1);
	}
	close(fd);
	// execute command 
	int e = execvp(ip[0],ip);
	if(e==-1){
		exit(-1);
	}
}

//execute for output redirectinon command 
void run_opredirection(char *op[],int index,int type){
	char *op_file = op[index+1];
	op[index]=NULL;
	int fd;
	if(type==2){
		// write content to the end of file
		fd = open(op_file, O_CREAT | O_WRONLY | O_APPEND,0644);
	}else{
		// trucate the file and write content to the file
		fd = open(op_file, O_CREAT | O_WRONLY | O_TRUNC,0644);
	}
	if(fd==-1){
		exit(-1);
	}
	// dup2 refer standard output to file
	if(dup2(fd, 1)==-1){
		exit(-1);
	}
	close(fd);
	// execute command
	int e = execvp(op[0],op);
	if(e==-1){
		exit(-1);
	}
}


//Method for checking input or ouput redirection and call method
void run_redirection(char *cmds[],int cmd_count){
	int chld=fork();
	if(chld==-1){
		printf("Error while forking \n");
		exit(-1);
	}else if(chld==0){
		// child process
		for(int i=0;i<cmd_count;i++){
			if(strcmp(cmds[i],"<")==0){
				run_ipredirection(cmds,i,0);
			}else if(strcmp(cmds[i],">")==0){
				run_opredirection(cmds,i,1);
			}else if(strcmp(cmds[i],">>")==0){
				run_opredirection(cmds,i,2);
			}
		}
	}else{
		// parent process
		int status;
		waitpid(chld,&status,0);
		if(WIFEXITED(status) &&  WEXITSTATUS(status)!=0){
			exit(-1);
		}
	}

}

// prepared command array provided by user sepearated by delimeter also validate the command length
void prepare_cmd(char *input,char *cmds[6], char *delimeter, int *len){
	char *cmd = strtok(input,delimeter);
	while(cmd!=NULL){
		if(*len>4){
			printf(" Too many arguments !!\n");
			*len=0;
			exit(-1);
			return;
		}
		cmds[(*len)++]=cmd;
		cmd=strtok(NULL,delimeter);
	}
	cmds[*len]=NULL;
}


// method for execute  conditional operator such as && and ||  and combination of both 
void run_conditional(char *input_cmd){

	int cmd_count=0;
	char *cmd ;
	char *last_op=NULL;
	int ls;
	// iterate through command entered by user
	while((cmd = strsep(&input_cmd,"||&&"))!=NULL){
		if(cmd_count>4){
			printf("Too many arguments \n");
			exit(-1);
		}

		if(*cmd == '\0'){
			continue;
		}
		char *args[6];
		char *arg;
		int argc=0;
		while((arg = strsep(&cmd," ")) != NULL){
			if(argc>4){
				printf("Too many arguments \n");
				exit(-1);
			}
			if(*arg=='\0'){
				continue;
			}
			args[argc++]=arg;
		}
		args[argc]=NULL;
		// if last operator is not null
		if(last_op!=NULL){
			// if last operator is conditional OR and last processs executed is success
			if(strcmp(last_op,"||")==0 && ls==0){
				break;
			// if last operator is conditional AND and last processs executed is failed
			}else if(strcmp(last_op,"&&")==0 && ls!=0){
				break;
			}
		}
		int status = run_process(args);
		if(input_cmd!=NULL){
			// update the last operator
			if(input_cmd[0]=='|'){
				last_op = "||";
			}else if(input_cmd[0]=='&'){
				last_op = "&&";
			}
		}
		// update the last status
		ls = status;
		// increment the command count
		cmd_count++;
	}
}




// method for opening a new session file.
void new_bash(){
	// fork to create child process
	int chld = fork();
	// if chld is less than 0 there is error while forking
	if(chld<0){
		exit(-1);
	// chld == 0  child process code
	}else if(chld==0){
		// open new terminal
		char *cmd = "gnome-terminal -- bash -c \"shell24\"";
		system(cmd);
		exit(0);
	}else{
		printf("Opening a new session \n");
	}
}

void run_shell(char *input_cmd){
	if(strcmp(input_cmd,"newt")==0){
		new_bash();
	}else if(strchr(input_cmd,'#')!=NULL){
		char *cmds[6];
		int len=0;
		prepare_cmd(input_cmd,cmds,"#",&len);
		if(len!=0)
			file_concat(cmds,len);
	}else if(strchr(input_cmd,'|')!=NULL && strstr(input_cmd,"||") == NULL){
		char *cmds[6];
		int len=0;
		prepare_cmd(input_cmd,cmds,"|",&len);
		if(len!=0)
			run_pipes(cmds,len);
	}else if(strchr(input_cmd,'>')!=NULL ||  strchr(input_cmd,'<')!=NULL || strcmp(input_cmd,">>")==0){
		char *cmds[6];
		int len=0;
		prepare_cmd(input_cmd,cmds," ",&len);
		run_redirection(cmds,len);
	}else if(strstr(input_cmd,"||")!=NULL || strstr(input_cmd,"&&")!=NULL){
			run_conditional(input_cmd);
	}
	else if(strchr(input_cmd,';')!=NULL){
		char *cmds[6];
		int len=0;
		prepare_cmd(input_cmd,cmds,";",&len);
		if(len!=0)
			run_sequence(cmds,len);
	}else if(strchr(input_cmd,'&')!=NULL){
		char *cmds[6];
		int len=0;
		char *cmd = strtok(input_cmd," ");
		while(cmd != NULL){
			cmds[len++]=cmd;
			cmd = strtok(NULL," ");
		}
		run_background(cmds, len);
	}else if(strcmp(input_cmd,"fg")==0){
		if(b_count>0)
			waitpid(b_pid[b_count],NULL,0);
		else
			printf("No more background process \n");
	}else{	
		char *cmds[6];
		int len=0;
		prepare_cmd(input_cmd,cmds," ",&len);
		if(len!=0)
			run_process(cmds);
	}
}


// Main method of Shell24
// Take input from user 
// Continuously Running run command 
int main(int argc,char *argv[]){
	// character array to hold input command provided by the user
	char input_command[2048];
	// to hold the background process id
	int b_pid[1024];
	// to hold the background process count
	int b_count=0;
	// infinit loop
	while('a'=='a'){
		printf("\nfork\n");
		int chld = fork();
		if(chld==-1){
			exit(-1);
		}else if(chld==0){
			while(1){
			printf("\nshell24$ " );
			fflush(stdout);
			if(fgets(input_command,sizeof(input_command),stdin)==NULL){
				exit(EXIT_FAILURE);
			}
			// make last new line character to blank character
			input_command[strlen(input_command)-1]='\0';
			// if input_command size is zero continue to while loop to read next input from user.
			if(strlen(input_command)==0)
				continue;

			run_shell(input_command);
			}

		}else{
			int status;
			waitpid(chld,&status,0);
		}
	}
	return 0;
}
