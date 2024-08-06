#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

//递归进行逐步质数筛选
void child_doing(int father_to_child[])
{
    close(father_to_child[1]);//关闭写方向

    int first;//第一数一定为质数

    int judge=read(father_to_child[0],&first,sizeof(first));

    if(judge==0)
    {
        close(father_to_child[0]);
        exit(0);
    }//左侧读取不到，直接关闭

    else
    {
        printf("prime %d\n",first);//第一个直接输出

        int child_to_grandson[2];
        pipe(child_to_grandson);//儿子到孙子管道

        int pid=fork();
        //子程序进入递归
        if(pid==0)
        child_doing(child_to_grandson);

        //父程序给下一代传筛选
        else
        {
            close(child_to_grandson[0]);

            int x=0;
            while(read(father_to_child[0],&x,sizeof(x))!=0)
            {
                if(x%first!=0)
                write(child_to_grandson[1],&x,sizeof(x));
            }//不被第一个数整除的进入下一代

            close(child_to_grandson[1]);
            wait(0);
        }

    }

    exit(0);

}


void main(int argc, char *argv[])
{
    int father_to_child[2];
    pipe(father_to_child);
    int pid=fork();

//子进程
    if(pid==0)
    child_doing(father_to_child);

//父进程
    else
    {
        close(father_to_child[0]);
        for(int i=2;i<=35;i++)
        {
            write(father_to_child[1],&i,sizeof(i));
        }//依次写入2-35给子进程

        close(father_to_child[1]);//写完关闭管道，节省资源
        wait(0);//等待子进程结束
    }

    exit(0);
}
