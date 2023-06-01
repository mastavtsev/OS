#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main() {

    char buf[6] = "Hello!";
    
    int depth = 1;
    char linkName[5] = "0.txt";
    
    char file[10] = "1";
	
	// Create a directory to store the files
    if (mkdir("test_dir", 0777) == -1) {
        perror("mkdir failed");
        return 1;
    }

    // Change to the test directory
    if (chdir("test_dir") == -1) {
        perror("chdir failed");
        return 1;
    }
    
    // Create a regular file
    int fd = open(linkName, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
        perror("fopen failed");
        return 1;
    }
    
    write(fd, buf, 6);
    close(fd);

    
    while (1) {
        // Create a symbolic link
        if (symlink(linkName, file) == -1) {
            printf("symlink failed\nRecursion depth: %d\n", depth);
            return 1;
        }
        
        // Attempt to open the symbolic link
        fd = open(file, O_RDONLY);
        if (fd == -1) {
            printf("open failed");
            break;
        }
        
        // Close the file descriptor
        close(fd);
        sprintf(linkName, "%d", depth);
        ++depth;
        sprintf(file, "%d", depth);
    }

    printf("Recursion depth: %d\n", depth);

    // Remove the test directory and its contents
    if (chdir("..") == -1) {
        perror("chdir failed");
        return 1;
    }
    if (rmdir("test_dir") == -1) {
        perror("rmdir failed");
        return 1;
    }

    return 0;
}

