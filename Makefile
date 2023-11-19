Output: Batch2

Batch2:
	@clear
	@gcc admin.c cpu.c memory.c swap.o term.o clock.c idle.c paging.o submit.c system.c loader.o process.c -pthread -no-pie -w -o  batch2.exe
	@./batch2.exe