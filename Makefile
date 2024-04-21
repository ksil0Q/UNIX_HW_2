build:
	gcc ./task.c -o task

config:
	touch statistics.log
run:
	./task -f filename 