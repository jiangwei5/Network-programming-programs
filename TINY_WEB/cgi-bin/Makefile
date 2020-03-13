CC = gcc
CFLAGS = -O2 -Wall -I ..
# -Wall是所有的警告信息都要提示，这样编译出来的程序很健壮；
# -O2表示编译时使用2级优化
# -I 指定库文件
all: adder

adder: adder.c
	$(CC) $(CFLAGS) -o adder adder.c

clean:
	rm -f adder *~
