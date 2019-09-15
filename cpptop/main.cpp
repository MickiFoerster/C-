#include <iostream>
#include <stdio.h>

float getCpuLoad() {
    float f = -1.0;
    FILE* fd = fopen("/proc/stat", "r");
    if (fd) {
        const size_t bufsz = 4096;
        char buf[bufsz];
        auto n = fread(buf, sizeof(buf[0]), sizeof(buf), fd);
        if (n>0) {
            std::string fc{buf};
            auto match = fc.find("cpu "); 
            if (match!=std::npos) {
                r=fscanf()
                while ()
                /*
                size_t i = 0;
                while(i<bufsz && fc[i]!='\n') {
                    i++;
                }
                if (fc[i]=='\n') {
                    fc[i] = '\0';
                }
                */
            }
        }
        fclose(fd);
    }
    return f;
}

int main() {
    float cpuload = getCpuLoad();
    std::cout << cpuload << std::endl;

    return 0;
}