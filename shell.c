#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>

void read_file(const char *directory);

int main(int argc, char *argv[]) {
  while(true){
    setbuf(stdout, NULL);
    printf("$ ");
  
    char command[1024];

    if (fgets(command, sizeof(command), stdin) == NULL) {
      break;
    }
    //EXIT
    command[strcspn(command, "\n")] = '\0';
    if(strcmp(command, "exit") == 0){
      break;
    }
    //TYPE
    else if(strncmp(command, "type ", 5) == 0){

      if(strncmp(command + 5, "echo", 4) == 0){
        printf("echo is a shell builtin\n");
      }
      else if(strncmp(command + 5, "exit", 4) == 0){
        printf("exit is a shell builtin\n");
      }
      else if(strncmp(command + 5, "type", 4) == 0){
        printf("type is a shell builtin\n");
      }
      else if(strncmp(command + 5, "pwd", 3) == 0){
        printf("pwd is a shell builtin\n");
      }
      else if(strncmp(command + 5, "cd", 2) == 0){
        printf("cd is a shell builtin\n");
      }
      else if (strncmp(command + 5, "cat ", 4) == 0){
        printf("cat is a shell builtin\n");
      }
      else{
        char full[PATH_MAX];
        const char *path = getenv("PATH");
        int len;
        int found = 0;

        while(path && sscanf(path, "%[^:]%n", full, &len) == 1){
          snprintf(full, sizeof(full), "%.*s/%s", len, path, command + 5);

          //PERMISSION CHECK
          if(access(full, X_OK) == 0){
            printf("%s is %s\n", command + 5, full);
            found = 1;
            break;
          }

          path += len + (path[len] == ':');
        }
        if(!found){
          printf("%s: not found\n", command + 5);
        }
      }
    }
    //ECHO
    else if(strncmp(command, "echo ", 5) == 0){
    	bool in_squotes = false;
	    bool in_dquotes = false;
    	char output[1024];
	    char *pnt = output;
    	bool space = false;
    	size_t bs = 0;
    	for(size_t i = 5; i < strlen(command); i++){
    		if(command[i] == '\'' && !in_dquotes && bs % 2 == 0){
		     in_squotes = !in_squotes;
		     bs = 0;
		     continue;
    		}
		    if(command[i] == '\"' && !in_squotes && bs % 2 == 0){
    			in_dquotes = !in_dquotes;
    			bs = 0;
    			continue;
		    }
    		//SINGLE QUOTES
    		if(in_squotes){
		    	*pnt = command[i];
    			pnt++;
    			space = true;
		    }
    		//DOUBLE QUOTES
		    else if(in_dquotes){
		      if(command[i] != '\\' || bs % 2 == 1){
  		    	*pnt = command[i];
      			pnt++;
      			space = true;
      	  }
		    }
    		//NO QUOTES
		    else{
    			if((space && command[i] == ' ') || (bs % 2 == 1 && command[i] == ' ')){
		    		*pnt = command[i];
    				pnt++;
    				space = (bs % 2 == 1);
		    	}
    			else if((command[i] != '\\' || bs % 2 == 1) && command[i] != ' '){
		    		*pnt = command[i];
				    pnt++;
				    space = true;
    			}
		    }
		    if(command[i] == '\\') bs++; else bs = 0;
    	}
    	*pnt = '\0';
    	printf("%s\n", output);
    }
    //PWD
    else if(strcmp(command, "pwd") == 0){
      char buf[PATH_MAX];
      if(getcwd(buf, sizeof(buf)) != NULL) printf("%s\n", buf);
    }
    //CD
    else if(strncmp(command, "cd ", 3) == 0){
      struct stat sb;
      if(strncmp(command + 3, "~", 1) == 0){
        char *home = getenv("HOME");
        if(strlen(command) > 4){
          char dir[PATH_MAX];
          snprintf(dir, sizeof(dir), "%s/%s", home, command + 5);
          chdir(dir);
        }
        else{
          chdir(home);
        }
      }
      else if(stat(command + 3, &sb) == 0 && S_ISDIR(sb.st_mode)){
        chdir(command + 3);
      }
      else{
        printf("cd: %s: No such file or directory\n", command + 3);
      }
    }
    //CAT
    else if (strncmp(command, "cat ", 4) == 0 || strcmp(command, "cat") == 0) {
      bool in_squotes = false;
      bool in_dquotes = false;
      char directory[1024];
      char *pnt = directory;

      for (size_t i = 4; command[i] != '\0'; i++) {
          if (command[i] == '\\' && !in_squotes) {
              i++;
              if (command[i] != '\0') {
                  *pnt++ = command[i];
              } else {
                  break;
              }
              continue;
          }
  
          if (command[i] == '\'' && !in_dquotes) {
              in_squotes = !in_squotes;
              continue;
          }

          if (command[i] == '"' && !in_squotes) {
              in_dquotes = !in_dquotes;
              continue;
          }

          if (command[i] == ' ' && !in_squotes && !in_dquotes) {
              if (pnt > directory) {
                  *pnt = '\0';
                  read_file(directory);
                  pnt = directory;
              }
              continue;
          }

          *pnt++ = command[i];
        }
	
        if (pnt > directory) {
            *pnt = '\0';
            read_file(directory);
        }
    }

      //EXECUTABLE
      else {
        char command_copy[1024];
        strcpy(command_copy, command);

        char *args[64];
        int arg_count = 0;

        char *token = strtok(command_copy, " \t");
        while (token != NULL && arg_count < 63) {
          args[arg_count++] = token;
          token = strtok(NULL, " \t");
        }
        args[arg_count] = NULL;

        if (arg_count > 0) {
          char full_path[PATH_MAX];
          int found = 0;

          if (strchr(args[0], '/') != NULL) {
            if (access(args[0], X_OK) == 0) {
              strcpy(full_path, args[0]);
              found = 1;
            }
          }
          else {
            const char *path = getenv("PATH");
            int len;

            while (path && sscanf(path, "%[^:]%n", full_path, &len) == 1) {
              snprintf(full_path, sizeof(full_path), "%.*s/%s", len, path, args[0]);

              if (access(full_path, X_OK) == 0) {
                found = 1;
                break;
              }
              path += len + (path[len] == ':');
            }
          }

          if (found) {
            pid_t pid = fork();

            if (pid == 0) {
              execv(full_path, args);
              perror("execv fallita");
              exit(1);
            } else if (pid > 0) {
              int status;
              waitpid(pid, &status, 0);
            } else {
              perror("fork fallita");
            }
          } else {
            printf("%s: command not found\n", command);
          }
        }
      }
    }
    return 0;
}

void read_file(const char *directory) {
    if (directory == NULL || strlen(directory) == 0) {
        return;
    }

    struct stat sb;
    if (stat(directory, &sb) == 0 && S_ISDIR(sb.st_mode)) {
        printf("cat: %s: Is a directory\n", directory);
    }
    else {
        FILE* fptr = fopen(directory, "r");
        if (fptr == NULL) {
            printf("cat: %s: No such file or directory\n", directory);
        }
        else {
            int ch;
            while ((ch = fgetc(fptr)) != EOF) {
                putchar(ch);
            }
            fclose(fptr);
        }
    }
}
