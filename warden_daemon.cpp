#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

#include <vector>
#include <iostream>
#include <string> 
#include <fstream>

using namespace std;

int main() {
    pid_t pid, sid;

    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    umask(0);

    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    while (1) {
        ifstream list("template.tbl");
        string filename;
        vector <string> files_to_rm;
        int fd;
        int files_exist = 0;
        list >> filename;
        while (list >> filename) {
            fd = open(filename.c_str(), O_RDONLY);
            if (fd == -1) {
                continue;
            }

            files_exist++;

            int attr;
            if (ioctl(fd, FS_IOC_GETFLAGS, &attr) == -1) {
                return 1;
            }

            if ((attr & FS_IMMUTABLE_FL) != FS_IMMUTABLE_FL) {

                files_to_rm.push_back(filename);
            }

            if (close(fd) == -1) {
                fprintf(stderr, "close() failed: %s\n", filename);
            }
        }
        int len = files_to_rm.size();
        for (int i = 0; i < len; i++) {
            string command = "rm ";
            command += files_to_rm[i];
            system(command.c_str());
        }
    }
}


