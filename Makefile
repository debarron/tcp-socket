PREFIX_C=src/c
PREFIX_P=src/python

client:
	@echo ">> Generating the client"
	cp ${PREFIX_C}/client.c ./
	gcc -o client client.c -w
	rm client.c

server:
	@echo ">> Generating the server"
	cp ${PREFIX_P}/server.py ./
