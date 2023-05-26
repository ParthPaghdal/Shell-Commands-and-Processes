// Shell starter file
// You may make any changes to any part of this file.

#include <errno.h>
#include <libgen.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include<pwd.h>

#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)

#define EXIT_HELP "'exit' is a builtin command for exiting the shell program.\n"
#define PWD_HELP "'pwd' is a builtin command for displaying the current working directory.\n"
#define CD_HELP "'cd' is a builtin command for changing the current working directory.\n"
#define HIST_HELP "'history' is a builtin command for printing the 10 most recent commands.\n"
#define HELP_HELP "'help' is a builtin command for printing information on builtin commands.\n"
#define ARG_ERROR "ERROR: More arguements were provided than expected.\n"
#define IND_ERROR "ERROR: The given command index is either not recognized or does not match one of the 10 most recent commands.\n"

#define AR_ERROR "Error: invalid argument .\n"

#define HISTORY_DEPTH 10
int NUM_COMMANDS = 0;
char history[HISTORY_DEPTH][COMMAND_LENGTH];

#define BUFFER_SIZE 1024
static char buffer[BUFFER_SIZE];
char buffCheck[BUFFER_SIZE];

/**
 * Command Input and Processing
 */

void handle_SIGINT(int sig)
{
  // handle SIGINT (ctrl-c) case, print out all help messages
  signal(sig, handle_SIGINT);
  write(STDOUT_FILENO, "\n", strlen("\n"));
  write(STDOUT_FILENO, PWD_HELP, strlen(PWD_HELP));
  write(STDOUT_FILENO, CD_HELP, strlen(CD_HELP));
  write(STDOUT_FILENO, EXIT_HELP, strlen(EXIT_HELP));
  write(STDOUT_FILENO, HELP_HELP, strlen(HELP_HELP));
  write(STDOUT_FILENO, HIST_HELP, strlen(HIST_HELP));
}

// Add buff to history array
void add_to_hist(char *buff)
{
  // create buff2, which is the command num followed by a tab and then the user input
  char tmp[1024];
  char buff2[1024];
  strcpy(tmp, buff);
  sprintf(buff2, "%d\t", NUM_COMMANDS);
  strcat(buff2, tmp);

  // shift down all previous elements in history
  for (int i = 9; i > 0; i--)
  {
    strcpy(history[i], history[i - 1]);
  }

  // add buff2 to top of history
  strcpy(history[0], buff2);
  NUM_COMMANDS++;
}

// helper func that retrieves a command from history
char *get_cmd(int cmd_num)
{
  char *cur_cmd;
  char *num_str;
  char *cmd;
  int num;
  // iterate through history, check if each element matches the cmd_num arg
  for (int i = 0; i < 10; i++)
  {
    cur_cmd = strdup(history[i]);
    num_str = strtok(cur_cmd, "\t");
    num = atoi(num_str);
    if (num == cmd_num)
    {
      cmd = strtok(NULL, "\t");
      return cmd;
    }
  }
  // this error should not happen if function is called correctly
  return "ERROR: index not in history";
}

// print 10 most recent commands
void print_hist()
{
  int num_printed = 10;
  if (NUM_COMMANDS < 10)
  {
    num_printed = NUM_COMMANDS;
  }
  for (int i = 0; i < num_printed; i++)
  {
    write(STDOUT_FILENO, history[i], strlen(history[i]));
    write(STDOUT_FILENO, "\n", strlen("\n"));
  }
}

/*
 * Tokenize the string in 'buff' into 'tokens'.
 * buff: Character array containing string to tokenize.
 *       Will be modified: all whitespace replaced with '\0'
 * tokens: array of pointers of size at least COMMAND_LENGTH/2 + 1.
 *       Will be modified so tokens[i] points to the i'th token
 *       in the string buff. All returned tokens will be non-empty.
 *       NOTE: pointers in tokens[] will all point into buff!
 *       Ends with a null pointer.
 * returns: number of tokens.
 */
int tokenize_command(char *buff, char *tokens[])
{
  int token_count = 0;
  _Bool in_token = false;
  int num_chars = strnlen(buff, COMMAND_LENGTH);
  for (int i = 0; i < num_chars; i++)
  {
    switch (buff[i])
    {
    // Handle token delimiters (ends):
    case ' ':
    case '\t':
    case '\n':
      buff[i] = '\0';
      in_token = false;
      break;

    // Handle other characters (may be start)
    default:
      if (!in_token)
      {
        tokens[token_count] = &buff[i];
        token_count++;
        in_token = true;
      }
    }
  }
  tokens[token_count] = NULL;
  return token_count;
}

/**
 * Read a command from the keyboard into the buffer 'buff' and tokenize it
 * such that 'tokens[i]' points into 'buff' to the i'th token in the command.
 * buff: Buffer allocated by the calling code. Must be at least
 *       COMMAND_LENGTH bytes long.
 * tokens[]: Array of character pointers which point into 'buff'. Must be at
 *       least NUM_TOKENS long. Will strip out up to one final '&' token.
 *       tokens will be NULL terminated (a NULL pointer indicates end of
 * tokens). in_background: pointer to a boolean variable. Set to true if user
 * entered an & as their last token; otherwise set to false.
 */
 char pre_dir[COMMAND_LENGTH];
 void  save_dir(){
  char* result = getcwd(buffer, BUFFER_SIZE);
  if(result == NULL){
    write(STDOUT_FILENO, "Unable to get current directory\n", strlen("Unable to get current directory\n"));
    return;
  }
  strcpy(pre_dir, buffer);
 }
void read_command(char *buff, char *tokens[], _Bool *in_background)
{
  *in_background = false;

  // Read input
  int length = read(STDIN_FILENO, buff, COMMAND_LENGTH - 1);

  if ((length < 0) && (errno != EINTR))
  {
    perror("Unable to read command. Terminating.\n");
    exit(-1); /* terminate with error */
  }

  // Null terminate and strip \n.
  buff[length] = '\0';
  if (buff[strlen(buff) - 1] == '\n')
  {
    buff[strlen(buff) - 1] = '\0';
  }

  // add to history unless buff is a '!' history command
  strcpy(buffCheck, buff);
  if (buffCheck[0] != '!')
  {
    add_to_hist(buff);
  }

  // Tokenize (saving original command string)
  int token_count = tokenize_command(buff, tokens);
  if (token_count == 0)
  {
    return;
  }

  // Extract if running in background:
  if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0)
  {
    *in_background = true;
    tokens[token_count - 1] = 0;
  }
}

// alternate read command to be used on non-user inputted commands (eg. during '!!')
void rea(char *buff, char *tokens[], _Bool *in_background)
{
  *in_background = false;

  // Read input
  int length = strlen(buff);

  if ((length < 0) && (errno != EINTR))
  {
    perror("Unable to read command. Terminating.\n");
    exit(-1); /* terminate with error */
  }

  // Null terminate and strip \n.
  buff[length] = '\0';
  if (buff[strlen(buff) - 1] == '\n')
  {
    buff[strlen(buff) - 1] = '\0';
  }

  // add to history
  strcpy(buffCheck, buff);
  if (buffCheck[0] != '!')
  {
    add_to_hist(buff);
  }

  // Tokenize (saving original command string)
  int token_count = tokenize_command(buff, tokens);
  if (token_count == 0)
  {
    return;
  }

  // Extract if running in background:
  if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0)
  {
    *in_background = true;
    tokens[token_count - 1] = 0;
  }
}

/**
 * Main and Execute Commands
 */
int main(int argc, char *argv[])
{
  char input_buffer[COMMAND_LENGTH];
  char *tokens[NUM_TOKENS];
  char cwd[COMMAND_LENGTH];
  char *get_cmd_output;
  char command[COMMAND_LENGTH];

  //signal(SIGINT, handle_SIGINT);
  // handler for SIGINT (ctrl-c)
  
  

  
  while (true)
  {
    
    struct sigaction handler;
    handler.sa_handler = handle_SIGINT;
    handler.sa_flags = 0;
    sigemptyset(&handler.sa_mask);
    sigaction(SIGINT, &handler, NULL);

    // Get command
    // Use write because we need to use read() to work with
    // signals, and read() is incompatible with printf().
    getcwd(cwd, COMMAND_LENGTH);
    write(STDOUT_FILENO, getcwd(cwd, COMMAND_LENGTH),
          strlen(cwd));
    write(STDOUT_FILENO, "$ ", strlen("$ "));
    _Bool in_background = false;
    read_command(input_buffer, tokens, &in_background);

    /**
     * Steps For Basic Shell:
     * 1. Fork a child process
     * 2. Child process invokes execvp() using results in token array.
     * 3. If in_background is false, parent waits for
     *    child to finish. Otherwise, parent loops back to
     *    read_command() again immediately.
     */
    // Fork a child process

    // Check to see if exit is called, exit if so
    if (strcmp(tokens[0], "exit") == 0)
    {
      if (tokens[1] != 0)
      {
        write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
      }
      else
      {
        exit(0);
      }
    }

    // Check to see if pwd is called, print current wd if so
    else if (strcmp(tokens[0], "pwd") == 0)
    {
      if (tokens[1] != 0)
      {
        write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
      }
      else
      {
        getcwd(input_buffer, COMMAND_LENGTH);
        write(STDOUT_FILENO, getcwd(input_buffer, COMMAND_LENGTH),
              strlen(getcwd(input_buffer, COMMAND_LENGTH)));
        write(STDOUT_FILENO, "\n", strlen("\n"));
      }
    }

    // Check to see if cd is called, change dir if applicable
    else if (strcmp(tokens[0], "cd") == 0)
    {
      char *dir = NULL;
      if (tokens[2] != 0)
      {
        write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
        continue;
      }
      if(tokens[1] == NULL || strcmp(tokens[1], "~") == 0){
        struct passwd *pwd = getpwuid(getuid());
        dir = pwd->pw_dir;
        save_dir();
        chdir(dir);
        continue;
      }
      if(strcmp(tokens[1], "-") == 0){
        dir = getcwd(buffer, BUFFER_SIZE);
        if(strcmp(dir, pre_dir) == 0){
          write(STDOUT_FILENO, "cd OLDPWD not set.\n", strlen("cd OLDPWD not set.\n"));
        }
        else{
          strcpy(dir, pre_dir);
          write(STDOUT_FILENO, dir, strlen(dir));
          write(STDOUT_FILENO, "\n", strlen("\n"));
          chdir(dir);
        }
        continue;
      }
      save_dir();

      
      
      {
        if (chdir(tokens[1]) != 0)
        {
          write(STDOUT_FILENO, "Please enter a valid filepath \n",
                strlen("Please enter a valid filepath \n"));
        }
      } 
    }

    // Check to see if help is called, print help messages according to second token (if any)
    else if (strcmp(tokens[0], "help") == 0)
    {
      if (tokens[2] != 0)
      {
        write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
      }
      else
      {
        if (tokens[1] == 0)
        {
          // print out all messages if no second token provided
          write(STDOUT_FILENO, PWD_HELP, strlen(PWD_HELP));
          write(STDOUT_FILENO, CD_HELP, strlen(CD_HELP));
          write(STDOUT_FILENO, EXIT_HELP, strlen(EXIT_HELP));
          write(STDOUT_FILENO, HELP_HELP, strlen(HELP_HELP));
          write(STDOUT_FILENO, HIST_HELP, strlen(HIST_HELP));
        }
        else
        {
          // print out messages corresponding to the second token
          if (strcmp(tokens[1], "exit") == 0)
          {
            write(STDOUT_FILENO, EXIT_HELP, strlen(EXIT_HELP));
          }
          else if (strcmp(tokens[1], "pwd") == 0)
          {
            write(STDOUT_FILENO, PWD_HELP, strlen(PWD_HELP));
          }
          else if (strcmp(tokens[1], "cd") == 0)
          {
            write(STDOUT_FILENO, CD_HELP, strlen(CD_HELP));
          }
          else if (strcmp(tokens[1], "history") == 0)
          {
            write(STDOUT_FILENO, HIST_HELP, strlen(HIST_HELP));
          }
          else if (strcmp(tokens[1], "help") == 0)
          {
            write(STDOUT_FILENO, HELP_HELP, strlen(HELP_HELP));
          }
          else
          {
            // error for invalid entry
            write(STDOUT_FILENO, "'", strlen("'"));
            write(STDOUT_FILENO, tokens[1], strlen(tokens[1]));
            write(STDOUT_FILENO, "' is an external command or application.\n", strlen("' is an external command or application.\n"));
          }
        }
      }
    }

    // Check to see if history is called, print previous 10 commands if so
    else if (strcmp(tokens[0], "history") == 0)
    {
      if (tokens[1] != 0)
      {
        write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
      }
      else
      {
        print_hist();
      }
    }

    // Check to see if a different history command is called, print an error or run the corresponding command
    else if (tokens[0][0] == '!')
    {
      if (tokens[1] != 0)
      {
        write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
      }
      if (tokens[0][1] == '!')
      {
        // !! case - re-run previous command
        if (strcmp(history[0], "") == 0)
        {
          // print an error if history is empty
          write(STDOUT_FILENO, IND_ERROR, strlen(IND_ERROR));
        }
        else
        {
          int index = NUM_COMMANDS - 1;

          // command is the previous command
          get_cmd_output = get_cmd(index);
          strcpy(command, get_cmd_output);
          rea(command, tokens, &in_background);

          // write(STDOUT_FILENO, tokens[0], strlen(tokens[0]));
          write(STDOUT_FILENO, get_cmd_output, strlen(get_cmd_output));
          write(STDOUT_FILENO, "\n", strlen("\n"));
          
          // run the command (similar format to above command checks)
          // exit check
          if (strcmp(tokens[0], "exit") == 0)
          {
            if (tokens[1] != 0)
            {
              write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
            }
            else
            {
              exit(0);
            }
          }

          // pwd check
          if (strcmp(tokens[0], "pwd") == 0)
          {
            if (tokens[1] != 0)
            {
              write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
            }
            else
            {
              getcwd(input_buffer, COMMAND_LENGTH);
              write(STDOUT_FILENO, getcwd(input_buffer, COMMAND_LENGTH),
                    strlen(getcwd(input_buffer, COMMAND_LENGTH)));
              write(STDOUT_FILENO, "\n", strlen("\n"));
            }
          }

          // cd check
          else if (strcmp(tokens[0], "cd") == 0)
          {
            if (tokens[2] != 0)
            {
              write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
            }
            else
            {
              if (chdir(tokens[1]) != 0)
              {
                write(STDOUT_FILENO, "Please enter a valid filepath \n",
                      strlen("Please enter a valid filepath \n"));
              }
            }
          }

          // help check
          else if (strcmp(tokens[0], "help") == 0)
          {
            if (tokens[2] != 0)
            {
              write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
            }
            else
            {
              if (tokens[1] == 0)
              {
                write(STDOUT_FILENO, PWD_HELP, strlen(PWD_HELP));
                write(STDOUT_FILENO, CD_HELP, strlen(CD_HELP));
                write(STDOUT_FILENO, EXIT_HELP, strlen(EXIT_HELP));
                write(STDOUT_FILENO, HELP_HELP, strlen(HELP_HELP));
                write(STDOUT_FILENO, HIST_HELP, strlen(HIST_HELP));
              }
              else
              {
                if (strcmp(tokens[1], "exit") == 0)
                {
                  write(STDOUT_FILENO, EXIT_HELP, strlen(EXIT_HELP));
                }
                else if (strcmp(tokens[1], "pwd") == 0)
                {
                  write(STDOUT_FILENO, PWD_HELP, strlen(PWD_HELP));
                }
                else if (strcmp(tokens[1], "cd") == 0)
                {
                  write(STDOUT_FILENO, CD_HELP, strlen(CD_HELP));
                }
                else if (strcmp(tokens[1], "history") == 0)
                {
                  write(STDOUT_FILENO, HIST_HELP, strlen(HIST_HELP));
                }
                else if (strcmp(tokens[1], "help") == 0)
                {
                  write(STDOUT_FILENO, HELP_HELP, strlen(HELP_HELP));
                }
                else
                {
                  write(STDOUT_FILENO, "'", strlen("'"));
                  write(STDOUT_FILENO, tokens[1], strlen(tokens[1]));
                  write(STDOUT_FILENO, "' is an external command or application.\n",
                        strlen("' is an external command or application.\n"));
                }
              }
            }
          }

          // history check
          else if (strcmp(tokens[0], "history") == 0)
          {
            if (tokens[1] != 0)
            {
              write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
            }
            else
            {
              print_hist();
            }
          }

          else if (strcmp(tokens[0], "ls") == 0 || strcmp(tokens[0], "echo") == 0 || strcmp(tokens[0], cwd) == 0) 
          {
            // avoid throwing error for external functions
          }

          else
          {
            // error if no matches
            write(STDERR_FILENO, AR_ERROR, strlen(AR_ERROR));
          }
        }
      }

      // !0 case - re-run command 0 if still in history
      else if (tokens[0][1] == '0')
      {
        // !0 case
        if (NUM_COMMANDS < 11 && NUM_COMMANDS > 0)
        {
          get_cmd_output = get_cmd(0);
          strcpy(command, get_cmd_output);
          rea(command, tokens, &in_background);
          write(STDOUT_FILENO, get_cmd_output, strlen(get_cmd_output));
          write(STDOUT_FILENO, "\n", strlen("\n"));

          // run the command (similar format to above command checks)
          // exit check
          if (strcmp(tokens[0], "exit") == 0)
          {
            if (tokens[1] != 0)
            {
              write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
            }
            else
            {
              exit(0);
            }
          }

          // pwd check
          if (strcmp(tokens[0], "pwd") == 0)
          {
            if (tokens[1] != 0)
            {
              write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
            }
            else
            {
              getcwd(input_buffer, COMMAND_LENGTH);
              write(STDOUT_FILENO, getcwd(input_buffer, COMMAND_LENGTH),
                    strlen(getcwd(input_buffer, COMMAND_LENGTH)));
              write(STDOUT_FILENO, "\n", strlen("\n"));
            }
          }

          // cd check
          else if (strcmp(tokens[0], "cd") == 0)
          {
            if (tokens[2] != 0)
            {
              write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
            }
            else
            {
              if (chdir(tokens[1]) != 0)
              {
                write(STDOUT_FILENO, "Please enter a valid filepath \n",
                      strlen("Please enter a valid filepath \n"));
              }
            }
          }

          // help check
          else if (strcmp(tokens[0], "help") == 0)
          {
            if (tokens[2] != 0)
            {
              write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
            }
            else
            {
              if (tokens[1] == 0)
              {
                write(STDOUT_FILENO, PWD_HELP, strlen(PWD_HELP));
                write(STDOUT_FILENO, CD_HELP, strlen(CD_HELP));
                write(STDOUT_FILENO, EXIT_HELP, strlen(EXIT_HELP));
                write(STDOUT_FILENO, HELP_HELP, strlen(HELP_HELP));
                write(STDOUT_FILENO, HIST_HELP, strlen(HIST_HELP));
              }
              else
              {
                if (strcmp(tokens[1], "exit") == 0)
                {
                  write(STDOUT_FILENO, EXIT_HELP, strlen(EXIT_HELP));
                }
                else if (strcmp(tokens[1], "pwd") == 0)
                {
                  write(STDOUT_FILENO, PWD_HELP, strlen(PWD_HELP));
                }
                else if (strcmp(tokens[1], "cd") == 0)
                {
                  write(STDOUT_FILENO, CD_HELP, strlen(CD_HELP));
                }
                else if (strcmp(tokens[1], "history") == 0)
                {
                  write(STDOUT_FILENO, HIST_HELP, strlen(HIST_HELP));
                }
                else if (strcmp(tokens[1], "help") == 0)
                {
                  write(STDOUT_FILENO, HELP_HELP, strlen(HELP_HELP));
                }
                else
                {
                  write(STDOUT_FILENO, "'", strlen("'"));
                  write(STDOUT_FILENO, tokens[1], strlen(tokens[1]));
                  write(STDOUT_FILENO, "' is an external command or application.\n",
                        strlen("' is an external command or application.\n"));
                }
              }
            }
          }

          // history check
          else if (strcmp(tokens[0], "history") == 0)
          {
            if (tokens[1] != 0)
            {
              write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
            }
            else
            {
              print_hist();
            }
          }
          else
          {
            // error if no matches
            write(STDERR_FILENO, AR_ERROR, strlen(AR_ERROR));
          }

        }

        else if (strcmp(tokens[0], "ls") == 0 || strcmp(tokens[0], "echo") == 0 || strcmp(tokens[0], cwd) == 0) 
        {
          // avoid throwing error for external functions
        }

        else
        {
          // error if command 0 is no longer in history
          write(STDOUT_FILENO, IND_ERROR, strlen(IND_ERROR));
        }
      }

      // !n case, run command number n if still in history
      else
      {
        char *n_STR = strtok(tokens[0], "!");
        int n = atoi(n_STR);
        // if atoi == 0, then the string was not a valid int - print error
        if (n == 0)
        {
          write(STDOUT_FILENO, IND_ERROR, strlen(IND_ERROR));
        }
        // check if n is in history
        else if (n > NUM_COMMANDS - 11 && n < NUM_COMMANDS && n > 0)
        {
          get_cmd_output = get_cmd(n);
          strcpy(command, get_cmd_output);
          rea(command, tokens, &in_background);
          write(STDOUT_FILENO, get_cmd_output, strlen(get_cmd_output));
          write(STDOUT_FILENO, "\n", strlen("\n"));

          // run the n'th command, similar checks to above for command
          // exit check
          if (strcmp(tokens[0], "exit") == 0)
          {
            if (tokens[1] != 0)
            {
              write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
            }
            else
            {
              exit(0);
            }
          }

          // pwd check
          if (strcmp(tokens[0], "pwd") == 0)
          {
            if (tokens[1] != 0)
            {
              write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
            }
            else
            {
              getcwd(input_buffer, COMMAND_LENGTH);
              write(STDOUT_FILENO, getcwd(input_buffer, COMMAND_LENGTH),
                    strlen(getcwd(input_buffer, COMMAND_LENGTH)));
              write(STDOUT_FILENO, "\n", strlen("\n"));
            }
          }

          // cd check
          else if (strcmp(tokens[0], "cd") == 0)
          {
            if (tokens[2] != 0)
            {
              write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
            }
            else
            {
              if (chdir(tokens[1]) != 0)
              {
                write(STDOUT_FILENO, "Please enter a valid filepath \n",
                      strlen("Please enter a valid filepath \n"));
              }
            }
          }

          // help check
          else if (strcmp(tokens[0], "help") == 0)
          {
            if (tokens[2] != 0)
            {
              write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
            }
            else
            {
              if (tokens[1] == 0)
              {
                write(STDOUT_FILENO, PWD_HELP, strlen(PWD_HELP));
                write(STDOUT_FILENO, CD_HELP, strlen(CD_HELP));
                write(STDOUT_FILENO, EXIT_HELP, strlen(EXIT_HELP));
                write(STDOUT_FILENO, HELP_HELP, strlen(HELP_HELP));
                write(STDOUT_FILENO, HIST_HELP, strlen(HIST_HELP));
              }
              else
              {
                if (strcmp(tokens[1], "exit") == 0)
                {
                  write(STDOUT_FILENO, EXIT_HELP, strlen(EXIT_HELP));
                }
                else if (strcmp(tokens[1], "pwd") == 0)
                {
                  write(STDOUT_FILENO, PWD_HELP, strlen(PWD_HELP));
                }
                else if (strcmp(tokens[1], "cd") == 0)
                {
                  write(STDOUT_FILENO, CD_HELP, strlen(CD_HELP));
                }
                else if (strcmp(tokens[1], "history") == 0)
                {
                  write(STDOUT_FILENO, HIST_HELP, strlen(HIST_HELP));
                }
                else if (strcmp(tokens[1], "help") == 0)
                {
                  write(STDOUT_FILENO, HELP_HELP, strlen(HELP_HELP));
                }
                else
                {
                  write(STDOUT_FILENO, "'", strlen("'"));
                  write(STDOUT_FILENO, tokens[1], strlen(tokens[1]));
                  write(STDOUT_FILENO, "' is an external command or application.\n",
                        strlen("' is an external command or application.\n"));
                }
              }
            }
          }

          // history check
          else if (strcmp(tokens[0], "history") == 0)
          {
            if (tokens[1] != 0)
            {
              write(STDOUT_FILENO, ARG_ERROR, strlen(ARG_ERROR));
            }
            else
            {
              print_hist();
            }
          }
          else
          {
            write(STDERR_FILENO, AR_ERROR, strlen(AR_ERROR));
          }
        }

        else if (strcmp(tokens[0], "ls") == 0 || strcmp(tokens[0], "echo") == 0 || strcmp(tokens[0], cwd) == 0) 
        {
          // avoid throwing error for external functions
        }

        else
        {
          // error if n is not in history
          write(STDOUT_FILENO, IND_ERROR, strlen(IND_ERROR));
        }
      }
    }

    else if (strcmp(tokens[0], "ls") == 0 || strcmp(tokens[0], "echo") == 0 || strcmp(tokens[0], cwd) == 0) 
    {
      // avoid throwing error for external functions
    }

    // error if invalid input
    else
    {
      write(STDERR_FILENO, AR_ERROR, strlen(AR_ERROR));
    }

    if(strcmp(tokens[0], "exit") != 0 && strcmp(tokens[0], "cd") != 0 && strcmp(tokens[0], "pwd") != 0 && strcmp(tokens[0], "help") != 0 && strcmp(tokens[0], "history") != 0 && strcmp(tokens[0], "!!") != 0 && strcmp(tokens[0], "![:number:]") != 0) 
    {
      // fork a child
      pid_t pid = fork();
      if (pid < 0)
      {
        perror("fork");
        exit(1);
      }

      if (pid == 0)
      {
        // execute child
        execvp(tokens[0], tokens);
      }
      else
      {
        if (!in_background)
        {
          waitpid(pid, NULL, 0);
        }
        while (waitpid(-1, NULL, WNOHANG) > 0)
        {
          ;
        }
      }
    }
  }

  return 0;
}