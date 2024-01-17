#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUF_SIZE 90
#define PORT 20000

int main() {
    int cli_sock;
    struct sockaddr_in serv_addr;

    char buffer[BUF_SIZE];
    ssize_t bytes_read, bytes_sent, bytes_received, bytes_write;

    if ((cli_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Unable to create socket\n");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &serv_addr.sin_addr);
    serv_addr.sin_port = htons(PORT);

    if (connect(cli_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Unable to connect to server\n");
        exit(EXIT_FAILURE);
    }

    printf("Connected to Server\n");

    // File handling
    char filename[BUF_SIZE];
    int fd;
    while (1) {
        printf("Enter the file name: ");
        scanf("%s", filename);

        // Check if the file exists
        if (access(filename, F_OK) == 0) {
            break;
        } else {
            perror("File Not Found");
        }
    }


    // Open the file with read-only access
    fd = open(filename, O_RDONLY, 0666);
    if (fd < 0) {
        perror("Error opening file for reading");
        close(cli_sock);
        exit(EXIT_FAILURE);
    }
    int k;
    printf("Enter k : ");
    scanf("%d", &k);
    send(cli_sock, &k, sizeof(k), 0);
    printf("[-] %d\n", k);
    printf("Packet sent\n");


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


    printf("All data sent\n");

    close(fd);

    // receiving the encrypted file
    char new_file_name[BUF_SIZE];
    sprintf(new_file_name, "%s.enc", filename);
    fd = open(new_file_name, O_CREAT | O_WRONLY | O_TRUNC, 0666);
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

    close(cli_sock);
    return 0;
}
