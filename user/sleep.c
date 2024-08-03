#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void main(int argc, char *argv[])
{
   if(argc != 2){
		fprintf(2, "must 1 argument for sleep\n");
		exit(1);
	}//没传参数，错误退出

	int sleep_time = atoi(argv[1]);
	printf("(nothing happens for a little while)\n");
	sleep(sleep_time);
	exit(0);//正确退出
}
