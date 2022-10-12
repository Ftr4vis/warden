#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

static int protect(int, string, const int, int);

int protection_is_on;
int daemon_is_on = 0;

int main() {
    int attr = FS_IMMUTABLE_FL;
    int fd;

    while (1) {
        ifstream list("template.tbl");

        string passwordInput;
        cout << "Type the password" << endl;

        termios oldt;
        tcgetattr(STDIN_FILENO, &oldt);
        termios newt = oldt;
        newt.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        cin >> passwordInput;
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        sleep(1);

        unsigned int hashInput = 0;
        for (int i = 0; i < passwordInput.length(); i++) {
            hashInput += (char)(passwordInput[i]) * pow(129, i);
        }

        int hashTrue;
        list >> hashTrue;
        if (hashInput == hashTrue) { // password is "itmo"
            cout << "What do you want to do? 1 - protect, 0 - unprotect" << endl;
            int mode_to_set;
            cin >> mode_to_set;
            if (mode_to_set != 0 && mode_to_set != 1) {
                cout << "Bad mode_to_set number!" << endl;
                sleep(1);
                continue;
            }

            if (mode_to_set == 0 && daemon_is_on == 1) {
                system("pkill warden_daemon");
                daemon_is_on = 0;
            }

            string filename;
            while (list >> filename) {
                fd = open(filename.c_str(), O_RDONLY);
                if (fd == -1) {
                    continue;
                }
                
                if (protect(fd, filename, attr, mode_to_set) == 1) {
                    break;
                }
                
                if (close(fd) == -1) {
                    fprintf(stderr, "close() failed: %s\n", filename);
                }
            }

            if (mode_to_set == 1 && !daemon_is_on) {
                system("./warden_daemon");
                daemon_is_on = 1;
            }
        }
        else {
            cout << "Wrong password!" << endl;
            sleep(1);
        }
    }
    exit(EXIT_SUCCESS);
}

static int protect(int fd, string filename, const int newAttr, int mode_to_set) {
    int attr;
    if (ioctl(fd, FS_IOC_GETFLAGS, &attr) == -1) {
        printf("ioctl failed to get flags: %d = %s\n", errno, strerror(errno));
        return 1;
    }

    if ((attr & FS_IMMUTABLE_FL) == FS_IMMUTABLE_FL) {
        protection_is_on = 1;  // is protected
    }
    else {
        protection_is_on = 0;  // isn't protected
    }

    if (mode_to_set == 1) {  // 1 is for chattr +i
        attr |= newAttr;
    }
    else if (mode_to_set == 0) {  // 0 is for chattr  -i
        attr &= ~newAttr;
    }

    string command = "chmod ";
    if (protection_is_on == 1) {
        if (mode_to_set == 1) {
            cout << "Files are already protected" << endl;
            sleep(1);
            return 1;
        }
        else if (mode_to_set == 0) {
            if (ioctl(fd, FS_IOC_SETFLAGS, &attr) == -1) {
                printf("ioctl failed to set flags: %d = %s\n", errno, strerror(errno));
                return 1;
            }
            command += "744 " + filename;
            system(command.c_str());
        }
    }
    else if (protection_is_on == 0) {
        if (mode_to_set == 1) {
            command += "000 " + filename;
            system(command.c_str());
            if (ioctl(fd, FS_IOC_SETFLAGS, &attr) == -1) {
                printf("ioctl failed to set flags: %d = %s\n", errno, strerror(errno));
                return 1;
            }
        }
        else if (mode_to_set == 0) {
            cout << "Files are already unprotected" << endl;
            sleep(1);
            return 1;
        }
    }

    return 0;
}
