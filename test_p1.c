#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char** argv) {
    if (argc<2){
        printf(1,"Need input read time\n");
        exit();
    }
    

    char file[256];
    int read_time = atoi(argv[1]);
    int filed_read_time = 0;
    int read_count = getreadcount();
    printf(1, "readcount is %d \n", read_count);
    for (int i = 0;i < read_time;++i) {
        if (read(0, file, sizeof(file)) < 0) {
            filed_read_time++;
        }
    }
    int new_read_count = getreadcount();
    if (new_read_count == read_count + read_time - filed_read_time) {
        printf(1, "Right read count.readcount is %d ,filed read time is %d, real read time is %d\n", new_read_count, filed_read_time, read_time - filed_read_time);
    }
    else {
        printf(1, "Wrong read count.readcount is %d ,filed read time is %d, real read time is %d\n", new_read_count, filed_read_time, read_time - filed_read_time);
    }
    exit();
}


