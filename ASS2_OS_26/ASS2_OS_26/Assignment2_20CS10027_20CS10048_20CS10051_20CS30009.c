#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glob.h>
#include <signal.h>
#include <dirent.h>

#define SIZE 200
#define CMD_SIZE 1024
#define TOKEN_SIZE 100
#define cursorforward(x) printf("\033[%dG",x+2);
#define cursorbackward(x) printf("\033[3G")

#define KEY_ESCAPE  0x001b
#define KEY_ENTER   0x000a
#define KEY_UP      0x0105
#define KEY_DOWN    0x0106
#define KEY_LEFT    0x0107
#define KEY_RIGHT   0x0108
#define CTRL_KEYPRESS(k) ((k)  & 0x1f)

char present[1024];

pid_t p;
int pos;
int my_cool_readline_func1 (int count, int key) {
    FILE *fp;
    fp = fopen(present,"r");
    int linesCount = 0;
    char ch;
   //read character by character and check for new line
    while((ch=getc(fp))!=EOF) {
        if(ch == '\n')
            linesCount++;
    }
    if(pos == 1000 || pos > linesCount){
        return 0;
    }
    close(fp);

    fp = fopen(present,"r");
    int i = 0;
    pos++;
    char c[100];
    while(fgets(c, 100, fp)){
        i++;
        if(i == pos) break;
        for(int j = 0;j < 100;j++) c[j] = '\0';
    }
    c[strlen(c)-1] = '\0';
    rl_replace_line(c,0);
    rl_redisplay();
    rl_end_of_line(count,key);
    close(fp);
    return 0;
}
int my_cool_readline_func2 (int count, int key) {
    if(pos <= 1){
        return 0;
    }
    int i = 1;
    char c[100];
    FILE *fp;
    fp = fopen(present,"r");
    while(fgets(c, 100, fp)){
        i++;
        if(i == pos) break;
        for(int j = 0;j < 100;j++) c[j] = '\0';
    }
    pos--;
    c[strlen(c)-1] = '\0';
    rl_replace_line(c,0);
    rl_redisplay();
    rl_end_of_line(count,key);
    close(fp);
    return 0;
}

int move_to_start_end (int count, int key) {
    if(key==5) rl_end_of_line(count,key);
    if(key==1) rl_beg_of_line(count,key);

    return 0;
}

void ctrl_c_handler(int sig) {
  
  
  printf("\nchild process terminated......\n");
  
  if(p!=0){
  kill(p,SIGKILL);
}


}
void ctrl_z_handler(int sig) {
    
    if(p ==0 ) {
        
        printf("\nChild process in bg.....\n");
        signal(SIGTSTP, SIG_DFL);
        raise(SIGTSTP);
        
    }
   /* else {
     printf("\nparent process\n");//child process will to background and parent process will run
      }*/
    
}
int buf_size;
void wildcard(char *pattern, char **tokens, int *size) {
    glob_t results;
    int i, j;

    if (glob(pattern, 0, NULL, &results) != 0) {
        fprintf(stderr, "Error: failed to match pattern %s\n", pattern);
        return;
    }

    for (j = 0; j < results.gl_pathc; j++) {
        tokens[(*size)++] = strdup(results.gl_pathv[j]);
        if ((*size) >= buf_size)
			{
				buf_size += TOKEN_SIZE;
				tokens = (char **)realloc(tokens, buf_size * sizeof(char *));
			}
    }

    globfree(&results);
}

int pipe_split(char *ch, char ***cmds)
{
	int size = 0;
	int buf_size = CMD_SIZE;
	*cmds = (char **)malloc(sizeof(char *) * buf_size);

	char *cmd;
	cmd = strtok(ch, "|");
	while (1)
	{

		if (cmd == NULL)
			break;
		(*cmds)[size++] = cmd;
		if (size >= buf_size)
		{
			buf_size += CMD_SIZE;
			*cmds = (char **)realloc(*cmds, buf_size * sizeof(char *));
		}
		cmd = strtok(NULL, "|");
	}
	(*cmds)[size] = NULL;
	return size;
}

char **cmd_split(char *ch, int *token_count)
{
        
	int size = 0, token_size = 0;
	buf_size = TOKEN_SIZE;
	char **tokens = (char **)malloc(sizeof(char *) * buf_size);
	char *cmd_tokens = (char *)malloc(sizeof(char) * buf_size);
	for (int i = 0; i < buf_size; i++) cmd_tokens[i] = '\0';
	//printf("%s\n",ch);
	//printf("%s\n",cmd_tokens);
	// cmd = strtok(ch, " \t\r\n\a");
	for (int i = 0; ch[i] != '\0'; i++)
	{
		if (ch[i] == ' ' || ch[i] == '<' || ch[i] == '>')
		{
			if (cmd_tokens[0] != '\0')
			{	
        				if (strchr(cmd_tokens, '*') || strchr(cmd_tokens, '?')) {
            				wildcard(cmd_tokens, tokens, &size);
        			 	 } 
        				else {
        				  //printf("%s\n",cmd_tokens);
           		 			tokens[size++] = strdup(cmd_tokens);
      			  		}
    				}
			
			// printf("%s\n",(*tokens)[size-1]);
			for (int i = 0; i < buf_size; i++)
				cmd_tokens[i] = '\0';
			token_size = 0;
			if (size >= buf_size)
			{
				buf_size += TOKEN_SIZE;
				tokens = (char **)realloc(tokens, buf_size * sizeof(char *));
			}

			if (ch[i] == '<' || ch[i] == '>')
			{
				char *g = (char *)malloc(sizeof(char));
				g[0] = ch[i];
				(tokens)[size++] = g;
				// printf("%s\n",(*tokens)[size-1]);
				if (size >= buf_size)
				{
					buf_size += TOKEN_SIZE;
					tokens = (char **)realloc(tokens, buf_size * sizeof(char *));
				}
			}
			}

		else
		{

			cmd_tokens[token_size++] = ch[i];
			if (ch[i + 1] == '\0')
			{
				if (strchr(cmd_tokens, '*') || strchr(cmd_tokens, '?')) {
            				wildcard(cmd_tokens, tokens, &size);
        			 	 } 
        				else {
           		 			tokens[size++] = strdup(cmd_tokens);
      			  		}
			} // printf("%s\n",(*tokens)[size-1]);}
		}
		// cmd = strtok(NULL, "|");
	}

	(tokens)[size] = NULL;
	*token_count = size;
	for (int i = 0; i < buf_size; i++) cmd_tokens[i] = '\0';
	return tokens;
}

void run_cmd(char **args, int tc,char *s)
{
	int lt = 0, gt = 0, am = 0;
    int ind1, ind2, ind3;
    for (int i = 0; i < tc; i++)
    {
        if (strcmp(args[i], "<") == 0)
        {
            lt = 1;
            ind1 = i;
        }
        if (strcmp(args[i], ">") == 0)
        {
            gt = 1;
            ind2 = i;
        }
        if (strcmp(args[i], "&") == 0)
        {
            am = 1;
            ind3 = i;
        }
    }
    if (lt == 1 && gt == 1)
    {
        char** toks;
		int siz;
		toks = (char**)malloc(sizeof(char*)*(ind1+1));
		for(int i = 0;i < ind1;i++){
			int l = strlen(args[i]);
			toks[i] = (char*)malloc(sizeof(char)*l);
			strcpy(toks[i],args[i]);
		}
		toks[ind1] = NULL; 
		int p = fork();
		if(p == 0){
			int no1 = open(args[ind1+1], O_RDWR);
            dup2(no1, STDIN_FILENO);
            int no2 = open(args[ind2+1], O_RDWR);
            dup2(no2,STDOUT_FILENO);
            execvp(toks[0],toks);
            perror("EXECVP");
            exit(1);
		}
        if (p < 0)
        {
            perror("FORK");
            exit(1);
        }
        if (p > 0)
        {
            int status;
            waitpid(-1,&status,WUNTRACED);
            if (am == 0)
             waitpid(p, &status, 0);
        }
    }
    else if (lt == 1)
    {
        char** toks;
		int siz;
		toks = (char**)malloc(sizeof(char*)*(ind1+1));
		for(int i = 0;i < ind1;i++){
			int l = strlen(args[i]);
			toks[i] = (char*)malloc(sizeof(char)*l);
			strcpy(toks[i],args[i]);
		}
		toks[ind1] = NULL; 
		int p = fork();
		if(p == 0){
			int no1 = open(args[ind1+1], O_RDWR);
            dup2(no1, STDIN_FILENO);
            execvp(toks[0],toks);
            perror("EXECVP");
            exit(1);
		}
        if (p < 0)
        {
            perror("FORK");
            exit(1);
        }
        if (p > 0)
        {
            int status;
            waitpid(-1,&status,WUNTRACED);
          if (am == 0)
             waitpid(p, &status, 0);
        }
    }
    else if (gt == 1)
    {
        char** toks;
		int siz;
		toks = (char**)malloc(sizeof(char*)*(ind2+1));
		for(int i = 0;i < ind2;i++){
			int l = strlen(args[i]);
			toks[i] = (char*)malloc(sizeof(char)*l);
			strcpy(toks[i],args[i]);
		}
		toks[ind2] = NULL; 
		int p = fork();
		if(p == 0){
            int no2 = open(args[ind2+1], O_RDWR);
            dup2(no2,STDOUT_FILENO);
            execvp(toks[0],toks);
            perror("EXECVP");
            exit(1);
		}
        if (p < 0)
        {
            perror("FORK");
            exit(1);
        }
        if (p > 0)
        {
            int status;
            waitpid(-1,&status,WUNTRACED);
          if (am == 0)
             waitpid(p, &status, 0);
        }
    }
    else
    {
        int p = fork();
        if (p == 0)
        {
            char *args[20];
            args[0] = strtok(s, " \n");
            int j;
            for (j = 1; j < 20; j++)
            {
                args[j] = strtok(NULL, " \n");
                if (args[j] == NULL)
                    break;
            }
            execvp(args[0], args);
            perror("EXECVP");
            exit(1);
        }
        if (p < 0)
        {
            perror("FORK");
            exit(1);
        }
        if (p > 0)
        {
            int status;
            waitpid(-1,&status,WUNTRACED);
            if (am == 0)
               waitpid(p, &status, 0);
        }
    }
}
void pipe_launch_cmd(char **args, int tc, int i,int j,char* s)
{
	int lt = 0, gt = 0, am = 0;
	int ind1, ind2, ind3;
	for (int i = 0; i < tc; i++)
	{
		if (strcmp(args[i], "<") == 0)
		{
			lt = 1;
			ind1 = i;
		}
		if (strcmp(args[i], ">") == 0)
		{
			gt = 1;
			ind2 = i;
		}
		if (strcmp(args[i], "&") == 0)
		{
			am = 1;
			ind3 = i;
		}
	}
	int p = fork();
	if(p == 0){
		if(i != 0){
			dup2(i, 0);
			close(i);
		}
		if(j != 1){
			dup2(j,1);
			close(j);
		}
		char **toks;
		if(lt == 1 && gt == 1){
			toks = (char**)malloc(sizeof(char*)*(ind1+1));
			for(int j = 0;j < ind1;j++){
				toks[j] = (char*)malloc(sizeof(char)*strlen(args[j]));
				strcpy(toks[j],args[j]);
			}
			toks[ind1] = NULL;
			close(1);
			close(0);
			dup(open(args[ind1+1],O_RDWR));
			dup(open(args[ind2+1],O_RDWR));
			execvp(toks[0],toks);
		}
		else if(lt == 1){
			toks = (char**)malloc(sizeof(char*)*(ind1+1));
			for(int j = 0;j < ind1;j++){
				toks[j] = (char*)malloc(sizeof(char)*strlen(args[j]));
				strcpy(toks[j],args[j]);
			}
			toks[ind1] = NULL;
			close(0);
			dup(open(args[ind1+1],O_RDWR));
			execvp(toks[0],toks);
		}
		else if(gt == 1){
			toks = (char**)malloc(sizeof(char*)*(ind2+1));
			for(int j = 0;j < ind2;j++){
				toks[j] = (char*)malloc(sizeof(char)*strlen(args[j]));
				strcpy(toks[j],args[j]);
			}
			toks[ind2] = NULL;
			close(1);
			dup(open(args[ind2+1],O_RDWR));
			execvp(toks[0],toks);
		}
		else execvp(args[0],args);
		exit(1);
	}
	if (p < 0)
	{
		perror("FORK");
		exit(1);
	}
	if (p > 0)
	{
		int status;
		waitpid(-1,&status,WUNTRACED);
		if(am == 0) waitpid(p, &status, 0);
	}
	
}


int* print_pids_with_file_open(int * counts, int token_count,char *filename) {
    
    DIR *dirp;
    struct dirent *entry;
    char path[1024];
    char exe[1024];
   static int pids[100];
    int count =0;

    dirp = opendir("/proc");
    if (dirp == NULL) {
        perror("Failed to open /proc");
        return 0;
    }

    while ((entry = readdir(dirp)) != NULL) {
        if (entry->d_type != DT_DIR)
            continue;

        int pid = atoi(entry->d_name);
        if (pid <= 0)
            continue;

        sprintf(path, "/proc/%d/fd", pid);
        DIR *fd_dirp = opendir(path);
        if (fd_dirp == NULL)
            continue;

        while ((entry = readdir(fd_dirp)) != NULL) {
            sprintf(path, "/proc/%d/fd/%s", pid, entry->d_name);
            char link[1024];
            ssize_t len = readlink(path, link, sizeof(link));
            if (len == -1)
                continue;
            link[len] = '\0';
     
            if (strcmp(link, filename) == 0) {
                sprintf(exe, "/proc/%d/exe", pid);
                char exe_link[1024];
                ssize_t exe_len = readlink(exe, exe_link, sizeof(exe_link));
                if (exe_len == -1)
                    continue;
                exe_link[exe_len] = '\0';
                pids[count++] = pid;
                
                break;
                
            }
        }
        closedir(fd_dirp);
    }

    closedir(dirp);
    *counts = count;
    return pids;
}

int del_ep(int token_count, char **tokens){

   pid_t pid;
    int pipe_fd[2];
    int* pids;
    int counts = 0;
    
    if(access(tokens[1],F_OK)==-1) {printf("File doesn't exists\n"); return 1;}

    if (token_count != 2) {
        fprintf(stderr, "Usage: %s filename\n", tokens[0]);
        return 1;
    }

    if (pipe(pipe_fd) == -1) {
        perror("Pipe creation failed");
        return 1;
    }

    pid = fork();
    if (pid == -1) {
        perror("Error creating child process");
        return 1;
    } else if (pid == 0) {
        
        close(pipe_fd[0]);  

        pids = print_pids_with_file_open( &counts,token_count,tokens[1]);
        write(pipe_fd[1], &counts, sizeof(counts));
        write(pipe_fd[1], pids, counts * sizeof(int));

        close(pipe_fd[1]); 
        exit(0);
    } else {
        
        close(pipe_fd[1]);  

      
        int read_counts = 0;
        read(pipe_fd[0], &read_counts, sizeof(read_counts));

       
        int read_pids[read_counts];

        
        read(pipe_fd[0], read_pids, read_counts * sizeof(int));

        char permission[10];
        int k = 0;
        for (int i = 0; i < read_counts; i++) {
            printf("Give permission to kill %d id : YES / NO  ", read_pids[i]);
            scanf("%s", permission);
            fflush(stdin);

            if (strcmp(permission, "YES") == 0) {
                kill(read_pids[i], SIGKILL);
                k++;
            } else {
                break;
            }
        }

        if (k == read_counts ) {
            unlink(tokens[1]);
            printf("File deleted \n");
        }
        close(pipe_fd[0]);  
        wait(NULL);
        return 0;
    
}
return 0;

}
void cd_pwd_delep(char* ch, char** tokens, int token_count){
	
	if(strcmp(tokens[0],"cd")==0 ){
		if(token_count > 1){
		
		    char* s = strtok(ch," ");
		    s = strtok(NULL," ");
		    chdir(s);
		    
		}
		else if(token_count==1){
		    chdir(getenv("HOME"));
		}
	}
	
	else if(strcmp(tokens[0],"pwd")==0){
	           char s[1024];
	           getcwd(s,sizeof(s));
	           printf("%s\n",s);
	}
	else{
	 int k = del_ep(token_count,tokens);
	
	}

}


int main()
{
         signal(SIGINT, ctrl_c_handler);
         signal(SIGTSTP, ctrl_z_handler);
         
         getcwd(present,sizeof(present));

         strcat(present,"/hist.txt");
	while (1)
	{

		char **cmds;
		char **tokens;
		

		char t;
		int len,c,k=0;
		pos = 0;
        rl_command_func_t my_cool_readline_func1;
        rl_bind_key ('\t', my_cool_readline_func1);
        rl_bind_key (27, my_cool_readline_func1); /* ascii code for ESC */
        rl_command_func_t my_cool_readline_func2;
        rl_bind_key ('\t', my_cool_readline_func2);
        rl_bind_key (27, my_cool_readline_func2);
        rl_bind_keyseq ("\\e[A", my_cool_readline_func1);
        rl_bind_keyseq ("\\e[B", my_cool_readline_func2);
        char*ch = readline("$ ");
        int ln = strlen(ch);
        ch[strlen(ch)] = '\n';
        ch[ln+1] = '\0';
        FILE* file = fopen(present, "r");
        if (file == NULL) {
            printf("Unable to open file\n");
            return 1;
        }
        // Store the contents of the file in a character array
        char contents[1000];
        int i = 0;
        while (!feof(file)) {
            contents[i++] = fgetc(file);
        }
        contents[i - 1] = '\0';

        // Close the file
        fclose(file);
        // Open the file in write mode
        file = fopen(present, "w");
        if (file == NULL) {
            printf("Unable to open file\n");
            return 1;
        }

        // Write the string to the file
        fprintf(file, "%s", ch);

        // Write the contents of the file after the inserted string
        fprintf(file, "%s", contents);

        // Close the file
        fclose(file);
		ch[strlen(ch)-1] = '\0';
	     rl_command_func_t move_to_start_end;
	     rl_bind_key (1, move_to_start_end);
	     rl_bind_key (5, move_to_start_end); /* ascii code for ESC */
		int token_count = 0;
		int pipe_count = 0;

		int cmd_count = pipe_split(ch, &cmds);
		
		char *Ch = (char*)malloc(100*sizeof(char));
		if (cmd_count == 1)
		{ 
			tokens = cmd_split(ch, &token_count);
		  for(int j=0;j<100;j++) Ch[j]='\0';
			for(int i=0;i<token_count;i++) {
			 strcat(Ch,tokens[i]);
			 strcat(Ch," ");
			}
			if(strcmp(tokens[0],"cd")==0 || strcmp(tokens[0],"pwd")==0 || strcmp(tokens[0],"delep")==0){
			   cd_pwd_delep(Ch,tokens,token_count);
			   continue;
			}
			/*if(){
				int k = del_ep(token_count,tokens);
			}*/
            if(strcmp(tokens[0],"exit") == 0) return 0;
			if(strcmp(tokens[0],"sb") == 0 && token_count >= 2){
				int pid = atoi(tokens[1]);
				if(token_count  == 3){
					if(strcmp(tokens[2],"-suggest") == 0){
						maini(pid,1);
						continue;
					}

				}
				else if(token_count == 2){
					maini(pid,0);
					continue;
				}
			}
			run_cmd(tokens, token_count,Ch);
		}

		if (cmd_count > 1)
		{
			int FD[2], i;
			int in = 0;
			char ** t;
			for(i = 0;i < cmd_count - 1;i++){
				pipe(FD);
				t = cmd_split(cmds[i],&token_count);
				
				for(int i=0;i<100;i++) Ch[i]='\0';
			
				for(int i=0;i<token_count;i++) {
			 		strcat(Ch,t[i]);
			 		strcat(Ch," ");
					}
				
				pipe_launch_cmd(t,token_count,in,FD[1],Ch);
				close(FD[1]);
				in = FD[0];
			}
			t = cmd_split(cmds[i],&token_count);
			pipe_launch_cmd(t,token_count,in,1,Ch);
		}
		free(Ch);
                
		fflush(stdin);
        	fflush(stdout);
	}
}
