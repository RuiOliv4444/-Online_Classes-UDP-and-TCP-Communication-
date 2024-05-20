To execute the UDP connection:

server´s terminal:
gcc servidor.c -o server 
./server 12345 12348 utilizadores.txt

other terminal:
nc -v -u 127.0.0.1 12348

To execute the TCP connection: 

server´s terminal:
gcc servidor.c -o server 
./server 12345 12348 utilizadores.txt

client´s terminal:

gcc cliente_tcp.c -o client
./client 127.0.0.1 12345
