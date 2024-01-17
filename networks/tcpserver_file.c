// Server Code

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <wait.h>
#include <sys/stat.h>

#define BUF_SIZE 50
#define PORT 20000

void handle_client(struct sockaddr_in cli_addr, int cli_sock);
void encrypt(char* c, int k){
	if (c[0]>='A' && c[0]<='Z'){
		*c = 'A' + (*c-'A'+k)%26;
	}
	else if (*c>='a' && *c<='z'){
		*c = 'a' + (*c-'a'+k)%26;
	}
}

int main() {
    int sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr;   

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Unable to create socket\n");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Unable to bind\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Server listening ...\n");
    listen(sockfd, 5);
    socklen_t cli_len = sizeof(cli_addr);
	while(1){
	    if ((newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &cli_len)) < 0) {
	        perror("Unable to accept connection\n");
	        close(sockfd);
	        exit(EXIT_FAILURE);
	    }

	    int pid = fork();
	    if(pid < 0){
	    	perror("Forking error");
	    	exit(EXIT_FAILURE);
	    }
	    else if(pid == 0){
	    	close(sockfd);
	    	handle_client(cli_addr, newsockfd);
	    	exit(EXIT_SUCCESS);
	    	// close(newsockfd);
	    }
	    else {
	    	close(newsockfd);
	    }

	}
	while((waitpid(-1, NULL, 0))>0);
    return 0;
}


void handle_client(struct sockaddr_in cli_addr, int cli_sock){
	ssize_t bytes_received, bytes_written, bytes_read, bytes_sent;
	char buffer[BUF_SIZE];

	printf("Accepted connection from %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
	char filename[BUF_SIZE];

	int k=0;

    bytes_received = recv(cli_sock, &k, sizeof(k), 0);
    printf("[+] %d\n", k);
    printf("Packet received\n");
    printf("Received number from client: %d\n", k);


	sprintf(filename, "%s.%d.txt", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
    int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (fd < 0) {
        perror("Unable to open file for writing\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        bytes_received = recv(cli_sock, buffer, BUF_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }
        printf("[+] %.*s\n", (int)bytes_received, buffer);
        printf("Packet received\n");
        // Check for termination signal ('$' character)
        int flag=0;
        for(int i=0; i<bytes_received; i++){
        	if (buffer[i] == '$') {
	            printf("Received termination signal. Closing file\n");
	            flag=1;
	            break;
	        }
	        else {
	        	write(fd, &buffer[i], 1);
	        }
        }
        if (flag==1) break;
    }

    close(fd);
    printf("File received successfully.\n");

    // encrypting the file
    fd = open(filename, O_RDONLY, 0666);
    if (fd < 0) {
        perror("Error opening file for reading");
        close(cli_sock);
        exit(EXIT_FAILURE);
    }
    char new_file_name[BUF_SIZE];
    sprintf(new_file_name, "%s.enc", filename);
    int nfd = open(new_file_name, O_CREAT | O_WRONLY, 0666);
    if (nfd < 0) {
        perror("Error opening file for reading");
        close(cli_sock);
        exit(EXIT_FAILURE);
    }
    char ch[1];
    while (1) {
        bytes_read = read(fd, ch, 1);
        if (bytes_read <= 0) {
            break;
        }
        //printf("%c->", ch[0]);
        encrypt(ch, k);
        //printf("%c\n", ch[0]);
        write(nfd, &ch[0], 1);
    }
    close(fd);
    close(nfd);


    // sedning the encrypted file
    fd = open(new_file_name, O_RDONLY, 0666);
    if (fd < 0) {
        perror("Error opening file for reading");
        close(cli_sock);
        exit(EXIT_FAILURE);
    }
    while (1) {
        bytes_read = read(fd, buffer, BUF_SIZE);
        if (bytes_read <= 0) {
            break;
        }

        bytes_sent = send(cli_sock, buffer, bytes_read, 0);
        if (bytes_sent <= 0) {
            perror("Send error!!");
            exit(EXIT_FAILURE);
        }

        printf("[-] %.*s\n", (int)bytes_sent, buffer);
        printf("Packet sent\n");
    }

    // Send termination signal ('$' character)
    send(cli_sock, "$", 1, 0);
    close(fd);
    printf("File sent successfully.\n");
    
    printf("Closed connection from %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
    close(cli_sock);
	return;
}

