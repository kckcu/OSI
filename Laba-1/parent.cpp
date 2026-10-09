#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>

void print_msg(const std::string& msg) {
    write(STDOUT_FILENO, msg.c_str(), msg.length());
}

int main() {
    print_msg("Enter filename: ");
    
    std::string filename = "";
    char c;
    
    while (read(STDIN_FILENO, &c, 1) > 0) {
        if (c == '\n') break;
        filename += c;
    }

    int file_fd = open(filename.c_str(), O_RDONLY);
    if (file_fd < 0) {
        print_msg("error: failed to open file\n");
        exit(1);
    }

    int pipe1[2];
    if (pipe(pipe1) == -1) {
        print_msg("error: pipe failed\n");
        exit(1);
    }


    pid_t pid = fork();
    
    if (pid == -1) {
        print_msg("error: fork failed\n");
        exit(1);
    } 
    else if (pid == 0) {
        
        close(pipe1[0]); 
        
        dup2(file_fd, STDIN_FILENO); 
        close(file_fd);
        
        dup2(pipe1[1], STDOUT_FILENO); 
        close(pipe1[1]);

        char* args[] = {(char*)"./child", NULL};
        execv("./child", args);
        
        print_msg("error: execv failed\n");
        exit(1);
    } 
    else {
       
        close(pipe1[1]); 
        close(file_fd);  

        char buf[4096];
        ssize_t bytes;
        
        
        while ((bytes = read(pipe1[0], buf, sizeof(buf))) > 0) {
            write(STDOUT_FILENO, buf, bytes);
        }
        close(pipe1[0]);

        int status;
        waitpid(pid, &status, 0); 
        
       
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            print_msg("error: child exited with error (division by zero)\n");
            exit(1);
        }
    }
    return 0;
}