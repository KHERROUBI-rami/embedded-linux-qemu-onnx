#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define TILE_SIZE 49152

int main(void)
{
    int fd;
    unsigned char buffer[TILE_SIZE];
    ssize_t total = 0;
    ssize_t n;

    fd = open("/dev/tile", O_RDONLY);
    if (fd < 0) {
        perror("open /dev/tile");
        return 1;
    }

    while (total < TILE_SIZE) {
        n = read(fd, buffer + total, TILE_SIZE - total);

        if (n < 0) {
            perror("read /dev/tile");
            close(fd);
            return 1;
        }

        if (n == 0)
            break;

        total += n;
    }

    close(fd);

    printf("Read %zd bytes from /dev/tile\n", total);

    if (total != TILE_SIZE) {
        fprintf(stderr,
                "Error: expected %d bytes, received %zd bytes\n",
                TILE_SIZE, total);
        return 1;
    }

    printf("Tile read successfully\n");
    return 0;
}
