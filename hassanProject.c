#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>

int pipe_count=0, fd;
static char* args[512];
char input_buffer[1024];
char *cmd_exec[100];
int flag, len;
char cwd[1024];
pid_t pid;
int environmment_flag;
int output_redirection, input_redirection;
int pid, status;
char history_data[1000][1000];
char current_directory[1000];
char *input_redirection_file;
char *output_redirection_file;
extern char** environ;
static int command(int, int, int, char *cmd_exec);

void sigintHandler(int sig_num)
{
    signal(SIGINT, sigintHandler);
    fflush(stdout);
}
void clear()
{
  fd =0;
  flag=0;
  len=0;
  pipe_count=0;
  output_redirection=0;
  input_redirection=0;
  input_buffer[0]='\0';
  cwd[0] = '\0';
  pid=0;
  environmment_flag=0;
}
 

void environmment()
{
  int i =1, index=0;
  char env_val[1000], *value;
  while(args[1][i]!='\0')
              {
                   env_val[index]=args[1][i];
                   index++;
                    i++;
              }
  env_val[index]='\0';
  value=getenv(env_val);

  if(!value)
      printf("\n");
  else printf("%s\n", value);
}


void change_directory()
{
char *h="/home";   
if(args[1]==NULL)
        chdir(h);
else if ((strcmp(args[1], "~")==0) || (strcmp(args[1], "~/")==0))
        chdir(h);
else if(chdir(args[1])<0)
    printf("bash: cd: %s: No such file or directory\n", args[1]);

}

void parent_directory()
{
if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
         printf("%s\n", cwd );
        }
else
       perror("getcwd() error");


}
void echo_calling(char *echo_val)
{
  int i=0, index=0;
  environmment_flag=0;
  char new_args[1024],env_val[1000], *str[10];
  str[0]=strtok(echo_val," ");
  str[1]=strtok(NULL, "");
  strcpy(env_val, args[1]);
  if(str[1]==NULL)
         {
          printf("\n");
          return;
         } 
  if (strchr(str[1], '$')) 
                  {
                  environmment_flag=1;
                  }
  
  memset(new_args, '\0', sizeof(new_args));
  i=0; 
  while(str[1][i]!='\0')
    {
      if(str[1][i]=='"')
      {
      index=0;     
      while(str[1][i]!='\0')
          {
          if(str[1][i]!='"')
                {
                new_args[index]=str[1][i];
                 flag=1;
                index++;
                }
          i++;
          }             
      }
      else if(str[1][i]==39)
      {
      index=0;
      while(str[1][i]!='\0')
          {
          if(str[1][i]!=39)
                {
                new_args[index]=str[1][i];
                 flag=1;
                index++;
                }
          i++;
          }               
      }
      else if(str[1][i]!='"')
        {
          new_args[index]=str[1][i];
          index++;
          i++;
        }
      else i++;    
    }


new_args[index]='\0';
if((strcmp(args[1], new_args)==0)&&(environmment_flag==0))
    printf("%s\n", new_args);
else {
     strcpy(args[1], new_args);
      if(environmment_flag==1)
                {
                environmment();
                }
      else if(environmment_flag==0)
                {
                  printf("%s\n", new_args ); 
                }
    }  
}


static char* skipwhite(char* s)
{
  while (isspace(*s)) ++s;
  return s;
}
void tokenise_commands(char *com_exec)
{
int m=1;
args[0]=strtok(com_exec," ");       
while((args[m]=strtok(NULL," "))!=NULL)
        m++;
}

void tokenise_redirect_output(char *cmd_exec)
{
  char *o_token[100];
  char *new_cmd_exec1;  
  new_cmd_exec1=strdup(cmd_exec);
  int m=1;
  o_token[0]=strtok(new_cmd_exec1,">");       
  while((o_token[m]=strtok(NULL,">"))!=NULL)
          m++;
  o_token[1]=skipwhite(o_token[1]);
  output_redirection_file=strdup(o_token[1]); 
  tokenise_commands(o_token[0]);   
  
}
static int split(char *cmd_exec, int input, int first, int last)
{
    char *new_cmd_exec1;  
    new_cmd_exec1=strdup(cmd_exec);
   //else
      {
        int m=1;
        args[0]=strtok(cmd_exec," ");       
        while((args[m]=strtok(NULL," "))!=NULL)
              m++;
        args[m]=NULL;
        if (args[0] != NULL) 
            {

            if (strcmp(args[0], "quit") == 0) 
                    exit(0);


            if (strcmp(args[0], "exit") == 0) 
                    exit(0);
                    
            if(strcmp("cd",args[0])==0)
                    {
                    change_directory();
                    return 1;
                    }
            else if(strcmp("pwd",args[0])==0)
                    {
                    parent_directory();
                    return 1;
                    }
           
            }
        }
    return command(input, first, last, new_cmd_exec1);
}

void pause_command()
{
printf("Press ENTER key to Can write your commands\n");  
char ch;
scanf("%c",&ch); 

}


void with_pipe_execute()
{

int i, n=1, input = 0, first = 1;

cmd_exec[0]=strtok(input_buffer,"|");

while ((cmd_exec[n]=strtok(NULL,"|"))!=NULL)
      n++;
cmd_exec[n]=NULL;
pipe_count=n-1;
for(i=0; i<n-1; i++)
    {
      input = split(cmd_exec[i], input, first, 0);
      first=0;
    }
input=split(cmd_exec[i], input, first, 1);
input=0;
return;

}
static int command(int input, int first, int last, char *cmd_exec)
{
  int mypipefd[2], ret, input_fd, output_fd;
  ret = pipe(mypipefd);
  if(ret == -1)
      {
        perror("pipe");
        return 1;
      }
  pid = fork();
 
  if (pid == 0) 
  {
    if (first==1 && last==0 && input==0) 
    {
      dup2( mypipefd[1], 1 );
    } 
    else if (first==0 && last==0 && input!=0) 
    {
      dup2(input, 0);
      dup2(mypipefd[1], 1);
    } 
    else 
    {
      dup2(input, 0);
    }
    if (strchr(cmd_exec, '<') && strchr(cmd_exec, '>')) // manual page
            {
              input_redirection=1;
              output_redirection=1;

            }
   else if (strchr(cmd_exec, '<')) 
        {
          input_redirection=1;

        }
   else if (strchr(cmd_exec, '>')) 
        {
          output_redirection=1;
         tokenise_redirect_output(cmd_exec);
        }
   if(output_redirection == 1)
                {                    
                        output_fd= creat(output_redirection_file, 0644);
                        if (output_fd < 0)
                          {
                          fprintf(stderr, "Failed to open %s for writing\n", output_redirection_file);
                          return(EXIT_FAILURE);
                          }
                        dup2(output_fd, 1);
                        close(output_fd);
                        output_redirection=0;
                }
    if(input_redirection  == 1)
                  {
                         input_fd=open(input_redirection_file,O_RDONLY, 0);
                         if (input_fd < 0)
                          {
                          fprintf(stderr, "Failed to open %s for reading\n", input_redirection_file);
                          return(EXIT_FAILURE);
                          }
                        dup2(input_fd, 0);
                        close(input_fd);
                        input_redirection=0;
                  }
                  

    if (strcmp(args[0], "echo") == 0)
              {
              echo_calling(cmd_exec);
              } 

    else if  (strcmp(args[0], "pause") == 0)
              {
               pause_command();
              }   
    else if(execvp(args[0], args)<0) printf("%s: command not found\n", args[0]);
              exit(0);
  }
  else 
  {
     waitpid(pid, 0, 0);  
   }
 
  if (last == 1)
    close(mypipefd[0]);
  if (input != 0) 
    close(input);
  close(mypipefd[1]);
  return mypipefd[0];

}
void my_path()
{
  char shell[1000];
   if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
    	  strcpy(shell, "");
          strcat(shell, cwd);
          strcat(shell, "/myshell$ ");

          printf("%s", shell);
        }
   else
       perror("getcwd() error");

}

int main()
{   
    int status;
    char ch[2]={"\n"};
    getcwd(current_directory, sizeof(current_directory)); // getcwd to get upsolute path name
    signal(SIGINT, sigintHandler); // prevent ctrl + c
    while (1)
    {
      clear();
      my_path();// to get the current directory
      
      fgets(input_buffer, 1024, stdin); // input of characters and strings
      if(strcmp(input_buffer, ch)==0) // no inputs
            {
                   continue;
            }
 
      len = strlen(input_buffer);
      input_buffer[len-1]='\0';
      with_pipe_execute();
      waitpid(pid,&status,0);
    }  

return 0;
}
