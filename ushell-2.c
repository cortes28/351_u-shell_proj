#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

// bryan cortes

#define MAX_CMD_LINE_ARGS  128

int min(int a, int b) { return a < b ? a : b; }

//concurrency status (&)
int amp_status = 0;

// redirecting status 1 (<)
int redirect_status1 = 0;

// redirecting status 2 (>)
int redirect_status2 = 0;

// pipe status (|) if we were to detect '|', status = 1. Otherwise = 0
int pipe_status = 0;

// break a string into its tokens, putting a \0 between each token
//   save the beginning of each string in a string of char *'s (ptrs to chars)
int parse(char* s, char** argv) {
  const char break_chars[] = " \t;";
  char* p;
  int c = 0;
  int redirect_flag = 0;      // becomes 1 when we encounter either '<' or '>'
  /* TODO */    // write parser that breaks input into argv[] structure
  // e.g., cal  -h  2022\0      // would be
  //       |    |   |
  //       |  \0| \0|   \0      // '\0' where all the spaces are
  //       p0   p1  p2          // array of ptrs to begin. of strings ("cal", "-h", "2022")
  //                            // char* argv[]

   while(*s != '\0') {     // while it does not equal \0 terminator, the string hasn't reached the end

  
    // while it equals one of the below (not \0 as that will end the main while loop), exchange that location with
    // a \0 and increase the number of arguments found
    while (*s == ' ' || *s == '\t' || *s == '\n')   
      *s++ = '\0';            // once we put \0, we move the string one step to the right        

          // means & was found towards the end (such as cat prog.c &)
      if (*s == '&')  { amp_status = 1; }   

      // determine 
      if (*s == '<')  { 
        redirect_status1 = 1;
        redirect_flag = 1;
      }         
      else if (*s == '>') { 
        redirect_status2 = 1; 
        redirect_flag = 1;
      }
       
    // save the location of where the beginning of the token is and increase the counter of tokens
    // if amp_status is not 0, it will not append the '&' as that will be a flag to determine for it to run concurrently
    if(amp_status == 0 ) {   

     if( redirect_flag == 1) {    // we will not log either < or > into the char array
        redirect_flag = 0;        // set it back to 0 to still input other arguments past < or >
     }

      else {
        *argv++ = s;
         c++;
      }
    }

    // while s[i] does not equal any of the following, move along the string 's'
    while(*s != '\n' && *s != '\0' && *s != '\t' && *s != ' ') 
      s++;
   
  }

  // append towards the end of argv a \0
      // check if ampersand is there
    // if(strncmp(argv[c - 1], "&", 1) == 0) {
    //   free(argv[c - 1]);                                      // get rid of &
    //   argv[c - 1] = NULL;                                     // set a null pointer
    //   c--;                                                          // go down one since we remove ampersand

    // }


  // append towards the end of argv a \0
  *argv = NULL;

  return c;   // int argc
}


// execute a single shell command, with its command line arguments
//     the shell command should start with the name of the command
int execute(char* input) {
  int i = 0;
  char* shell_argv[MAX_CMD_LINE_ARGS];
  memset(shell_argv, 0, MAX_CMD_LINE_ARGS * sizeof(char));

  
  int shell_argc = parse(input, shell_argv);
  // printf("after parse, what is input: %s\n", input);      // check parser
  // printf("argc is: %d\n", shell_argc);
  // while (shell_argc > 0) {
  //   printf("argc: %d: %s\n", i, shell_argv[i]);
  //   --shell_argc;
  //   ++i;
  // }
  
  int status = 0;
  pid_t pid = fork();
  
  if (pid < 0) { fprintf(stderr, "Fork() failed\n"); }  // send to stderr
  else if (pid == 0) { // child
    int ret = 0;
    // if ((ret = execlp("cal", "cal", NULL)) < 0) {  // can do it arg by arg, ending in NULL


    //don't know if this will work
    //----------------------------------------------------------------------------------------//

    // if redirect_status1 = 1
    if(redirect_status1 ==  1) {    // we found '>' within the input (output)
      int fd_1 = open(shell_argv[1], O_CREAT | O_TRUNC | O_WRONLY, 0644);     // create file if not there, if its there we will get rid of its content, and write only
      
      // not properly redirecting directory... ex. ls < file -> type 'ls' and its  info would go to new file...further more, only get out with ctrl + Z
      if(dup2(fd_1, STDOUT_FILENO) < 0) {
        printf("Unable to duplicate file descriptor (>)\n");
      }     
      
      close(fd_1);
      redirect_status1 = 0;             // convert it back to 0
    }

    // if redirect_status2 = 1
    else if(redirect_status2 == 1) {    // found '<' within the input
      int fd_2 = open(shell_argv[1], O_RDONLY);               
      
      if(dup2(fd_2, STDIN_FILENO) < 0) {
        printf("Unable to duplicate file descriptor (<)\n");
      }

      close(fd_2);
      redirect_status2 = 0;             // convert it back to 0
    }
    // //----------------------------------------------------------------------------------------//

    else if ((ret = execvp(*shell_argv, shell_argv)) < 0) {
      fprintf(stderr, "execlp(%s) failed with error code: %d\n", *shell_argv, ret);
    }
    printf("\n");
  }
  else if (amp_status == 1) { // parent -----  don't wait if you are creating a daemon (background) process (to run concurrently meaning you are not waiting) that we found &
    while (wait(&status) != pid) { }
  }
  else                        // parent waits for child processes to finish before moving on
    while(wait(NULL) > 0);
  
  return 0;
}

char hist[BUFSIZ];

int main(int argc, const char * argv[]) {
  char input[BUFSIZ];
  char last_input[BUFSIZ];  
  bool finished = false;
  size_t in_size = 0;
  //
  

  memset(last_input, 0, BUFSIZ * sizeof(char));  
  while (!finished) {
    memset(input, 0, BUFSIZ * sizeof(char));

    // printf("\ninput size:%ld\n", strlen(input));

    printf("osh > ");
    
    fflush(stdout);


    // if (strlen(input) > 0) {
    //   strncpy(last_input, input, min(strlen(input), BUFSIZ));
    //   printf("\ninput was: '%s'\n", input);
    //   printf("last_input was: '%s'\n\n", last_input);

    //   memset(last_input, 0, BUFSIZ * sizeof(char));
    // }

    if ((fgets(input, BUFSIZ, stdin)) == NULL) {   // or gets(input, BUFSIZ);
      fprintf(stderr, "no command entered\n");
      exit(1);
    }

    input[strlen(input) - 1] = '\0';          // wipe out newline at end of string
    //  printf("input was: '%s'\n", input);
    //  printf("last_input was: '%s'\n", hist);

    if (strncmp(input, "exit", 4) == 0) {   // only compare first 4 letters
      finished = true;
    } else if (strncmp(input, "!!", 2) == 0) { // check for history command
      // TODO
      if(strlen(hist) > 0) {
        if (strncmp(hist, "!!", 2) == 0) {
          printf("!! was our last input and thus cannot execute previous history command\n");
        }
        else
          execute(hist);
      }
     
      else
        printf("No commands in history...\n");

    } else {

        memset(hist, 0, BUFSIZ * sizeof(char));
        strncpy(hist, input, min(strlen(input), BUFSIZ));

        // printf("\ninput was: '%s'\n", input);
        // printf("last_input was: '%s'\n\n", hist);
 
       execute(input);

        }

 
  }
  
  printf("\t\t...exiting\n");
  return 0;
}
