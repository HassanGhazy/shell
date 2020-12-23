#include <stdlib.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <ctype.h>

#define WRITE 1
#define READ 0

#define clear() printf("\033[H\033[J") // ANSI escape codes.
#define cursorforward(x) printf("\033[%dC", (x))
#define cursorbackward(x) printf("\033[%dD", (x))
#define backspace(x) printf("\033[%dP", (x))

#define KEY_ESCAPE 0x001b
#define KEY_ENTER 0x000a
#define KEY_UP 0x0105
#define KEY_DOWN 0x0106
#define KEY_RIGHT 0x0107
#define KEY_LEFT 0x0108
#define BACK_SPACE 0x007F

int pid;
int pid2;
static struct termios term, oterm;

char * trimwhitespace(char * str) {
  char * end;

  while (isspace((unsigned char) * str)) str++;

  if ( * str == 0)
    return str;

  end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char) * end)) end--;

  end[1] = '\0';

  return str;
}

static int getch(void) {
  int c = 0;

  tcgetattr(0, & oterm);
  memcpy( & term, & oterm, sizeof(term));
  term.c_lflag &= ~(ICANON | ECHO);
  term.c_cc[VMIN] = 1;
  term.c_cc[VTIME] = 0;
  tcsetattr(0, TCSANOW, & term);
  c = getchar(); // to read the first character or the next character
  tcsetattr(0, TCSANOW, & oterm);
  
  return c;
}

static int kbhit(void) {
  int c = 0;

  tcgetattr(0, & oterm);
  memcpy( & term, & oterm, sizeof(term));
  term.c_lflag &= ~(ICANON | ECHO);
  term.c_cc[VMIN] = 0;
  term.c_cc[VTIME] = 1;
  tcsetattr(0, TCSANOW, & term);
  c = getchar();
  tcsetattr(0, TCSANOW, & oterm);
  if (c != -1) ungetc(c, stdin);
  return ((c != -1) ? 1 : 0);
}

static int kbesc(void) {
  int c;

  if (!kbhit()) return KEY_ESCAPE;
  c = getch();
  if (c == '[') {
    switch (getch()) {
    case 'A':
      c = KEY_UP;
      break;
    case 'B':
      c = KEY_DOWN;
      break;
    case 'C':
      c = KEY_RIGHT;
      break;
    case 'D':
      c = KEY_LEFT;
      break;
    case 'P':
      c = BACK_SPACE;
      break;
    default:
      c = 0;
      break;
    }
  } else {
    c = 0;
  }
  if (c == 0)
    while (kbhit()) getch();
  return c;
}

static int kbget(void) {
  int c;

  c = getch();
  return (c == KEY_ESCAPE) ? kbesc() : c;
}
void pauseCommand()
{
printf("Press Enter to continue\n");  
char ch;
scanf("%c",&ch); 

}
int main(int argc, char * argv[]) {

  pid2 = getpid();
  pid = getpgid(pid2);
  char history[100][100];
  int history_index = 0;
  int c;
  clear();

  while (1) {

    char cwd[1000];
    char * currentPath; // to get default directory
    char * args[11];
    char command[100];
    int com_index = 0;
    int cursor_index = 0;
    int history_pointer = history_index;

    if (getcwd(cwd, sizeof(cwd)) != NULL) { // get the current Path
      currentPath = cwd;
    }
    strcat(currentPath, "/myshell$");
    printf("%s ", currentPath); //  default directory

    int argscounter = 0;

    while (1) {
      c = kbget();
      if (c == KEY_ESCAPE) {
        break;
      } else if (c == BACK_SPACE) {
        if (com_index != 0) {
          cursorbackward(1);
          backspace(1);
          com_index--;
          cursor_index--;
          command[com_index] = '\0';
        }
        continue;
      } else if (c == KEY_ENTER) {
        command[com_index] = '\0';
        printf("\n");
        break;
      } else if (c == KEY_UP) {
        if (history_pointer > 0) {
          history_pointer--;
          putchar(' ');
          com_index++;
          cursorbackward(cursor_index + 1);
          backspace(com_index);
          com_index = 0;
          cursor_index = 0;
          int length = 0;
          for (int i = 0; i < sizeof(history[history_pointer]); i++) {
            if (history[history_pointer][i] != 0) {
              length++;
            }
          }
          for (int i = 0; i < length; i++) {
            putchar(history[history_pointer][i]);
            command[com_index] = history[history_pointer][i];
            cursor_index++;
            com_index++;
          }
        }
      } else if (c == KEY_DOWN) {
        if (history_pointer < history_index) {
          history_pointer++;
          putchar(' ');
          com_index++;
          cursorbackward(cursor_index + 1);
          backspace(com_index);
          com_index = 0;
          cursor_index = 0;
          int length = 0;
          for (int i = 0; i < sizeof(history[history_pointer]); i++) {
            if (history[history_pointer][i] != 0) {
              length++;
            }
          }
          for (int i = 0; i < length; i++) {
            putchar(history[history_pointer][i]);
            command[com_index] = history[history_pointer][i];
            cursor_index++;
            com_index++;
          }
        }
      } else if (c == KEY_RIGHT) {
        if (cursor_index < com_index) {
          cursor_index++;
          cursorforward(1);
        }
      } else if (c == KEY_LEFT) {
        if (cursor_index > 0) {
          cursor_index--;
          cursorbackward(1);
        }
      } else {
        if (com_index < 100) {
          putchar(c);

          if (cursor_index < com_index) {
            int diff = com_index - cursor_index;
            int index = cursor_index;
            char temp = c;
            while (com_index + 1 != index) {
              char in_temp = command[index];
              command[index] = temp;
              temp = in_temp;
              if (index < com_index) {
                putchar(temp);
              }
              index++;
            }
            cursorbackward(diff);
          } else {
            command[com_index] = c;
          }

          cursor_index++;
          com_index++;
        }
      }
    
    }
  
    if (strcmp(command, "") == 0) {// no cammond
      continue;
    }

    strcpy(history[history_index], command);
    history_index++;
    char * command_token = strtok(command, " ");
    while (command_token != NULL && argscounter < 10) {

      if (argscounter != 0) {
        command_token[-1] = '\0';
      }

      args[argscounter] = command_token;

      command_token = strtok(NULL, " ");
      argscounter++;

    }
    args[argscounter] = NULL;
    
       
      if (strcmp(args[0], "pause") == 0) {
     
    pauseCommand();
    }

 
    if (strcmp(args[0], "exit") == 0) {
      exit(0);
    }

    if (strcmp(args[0], "quit") == 0) {

      exit(0);
      kill(pid, SIGKILL);
      kill(pid2, SIGKILL);
      execlp("exit", "exit", NULL);
    }


    int pindex = 0;
    int thereis = 0;
    while (args[pindex] != NULL) {
      if (strcmp(args[pindex], "|") == 0) {
        thereis = 1;
        break;
      }
      pindex++;
    }

    if (thereis) {

      if (fork() == 0) {
        int pipefd[2];
        pipe(pipefd);

        if (fork() == 0) {
        
          close(pipefd[WRITE]);
          dup2(pipefd[READ], 0);
          close(pipefd[READ]);
          execlp(args[pindex + 1], args[pindex + 1], args[pindex + 2], NULL);

        } else {
          if (pindex == 1) {
            close(pipefd[READ]);
            dup2(pipefd[WRITE], 1);
            close(pipefd[WRITE]);
            execlp(args[0], args[0], NULL);
          } else {
            close(pipefd[READ]);
            dup2(pipefd[WRITE], 1);
            close(pipefd[WRITE]);
            execlp(args[pindex - 2], args[pindex - 2], args[pindex - 1], NULL);
          }
          

        }
        for (size_t i = 0; i < 100; ++i)
          command[i] = 0;
        exit(0);

      } else {
    
        for (size_t i = 0; i < 100; ++i)
   
          wait(NULL);

      }

    } else /*if (strlen (args[0]) > 0) */ {

      if (strcmp(args[0], "cd") == 0) {

        if (chdir(args[1]) != 0) {
       
        }
       
      } else {

        if (fork() == 0) {

          if (execvp(args[0], args) == -1) {
            exit(0);
          }

          exit(0);
        } else {
          for (size_t i = 0; i < 100; ++i)
            command[i] = 0;
          wait(NULL);
         
        }

      }

    }


  }
  return 0;
}
