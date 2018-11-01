proxy_server: proxy.o proxy_server.o
	gcc -o proxy_server proxy.o proxy_server.o
	rm proxy_server.o proxy.o

proxy.o: proxy.c
	gcc -c proxy.c

proxy_server.o: proxy_server.c
	gcc -c proxy_server.c

clean:
	rm *.o proxy_server
