#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


char *get_current_name(char*path)
{
    char*i;
    for(i=path+strlen(path);i>=path&& *i!='/';i--);
    i++;
    //过滤父目录信息
    return i;
}



void judge_same(char *path,char* file_name)
{
    if(strcmp(get_current_name(path),file_name)==0)//实验手册提示使用strcmp比较才不会报错
    printf("%s\n",path);
}


void find(char *path,char *file_name)
{
    /*模仿 ls.c进行编写*/
     char buf[512], *p;
     int fd;
     struct dirent de;
     struct stat st;
    if((fd = open(path, 0)) < 0)
    {
    fprintf(2, "ls: cannot open %s\n",path);
    return;
    }

    if(fstat(fd, &st) < 0)
    {
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
    }

    switch(st.type)
    {
        case T_FILE:
        judge_same(path, file_name);
        break; //当前目录下进行查找

        case T_DIR:

        if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
        {
          printf("ls: path too long\n");
          break;
        }//文件路径超出范围判断

        strcpy(buf,path);//复制到buf
        p=buf+strlen(buf);
        *p++ = '/';//在路径后面加上'/'

        //如果是文件夹，则不断读取目录下所有文件
        while(read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if(de.inum==0 ||(strcmp(de.name, ".") == 0) || (strcmp(de.name, "..") == 0))
            continue;  //实验hint,不进入. ..递归


            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;

            find(buf,file_name);//进入子目录递归查找
        }
        break;
    }
    close(fd);

}



int main(int argc, char *argv[])
{
    if(argc!=3)
    {
        printf("wrong using!\nUsage: find <dirName> <filename>\n");
        exit(1);
    }
    else
    {
        find(argv[1],argv[2]);
        exit(0);
    }
}