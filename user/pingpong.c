#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void main(int argc, char *argv[])
{
   int father_to_child[2];
   int child_to_father[2];
   pipe(father_to_child);
   pipe(child_to_father);//建立两个反向管道
   char buf[20];

   int pid=fork();//建立子进程，0为新进程，否则为父进程

//子进程操作
   if(pid==0)
   {
    close(child_to_father[0]);
    close(father_to_child[1]); //子到父不读；父到子不写

  
    int x=read(father_to_child[0],buf,1);//读取父进程一个byte
    if(x==1)
    {
        printf("%d: received ping\n",getpid());
    }//读取成功输出信息

    write(child_to_father[1],buf,sizeof(buf));
    exit(0);

   }

//父进程操作
   else
   {
    close(father_to_child[0]);
    close(child_to_father[1]);//反之
    write(father_to_child[1],"ping",4);//发送ping给子进程

    int x=read(child_to_father[0],buf,1);
    if(x==1)
       {
        printf("%d: received pong\n",getpid());
       }//接受子进程信息并回复
    exit(0);
   }
}
