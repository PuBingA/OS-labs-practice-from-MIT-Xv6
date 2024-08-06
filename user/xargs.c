#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


#define max_size 512 //字符串最大限制

int main(int argc, char* argv[])
{
    char buf[max_size];
    int n;

    if (argc < 2) 
    {
        while((n = read(0, buf, sizeof (buf))) > 0) 
        {
            write(1, buf, n);
        }
        exit(0);
    } //没有添加，直接输出即可

    char* args[32];
    int numArg;
    for (int i = 1; i < argc; ++i) 
    {
        args[i-1] = argv[i];
    }
    numArg = argc - 1;
    char* p = buf;
    // 读取未添加之前的command

    while ((n = read(0, p, 1)) > 0) 
    {

        if (*p == '\n') //每一行结束，进行添加
        {
            *p = 0;
            if (fork() == 0) 
            {
                args[numArg] = buf;
                exec(args[0], args);
                exit(0);
            } 

            else 
                wait(0);

            p = buf;
        } 
        
        else 
        {
            ++p;
        }
    }

    
    exit(0);
}
