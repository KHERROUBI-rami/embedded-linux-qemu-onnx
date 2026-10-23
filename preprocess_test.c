#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define WIDTH 64
#define HEIGHT 64
#define CHANNELS 6
#define TILE_BYTES (WIDTH * HEIGHT * CHANNELS * sizeof(uint16_t))

int main(void)
{
    int fd;
    uint16_t hwc[WIDTH * HEIGHT * CHANNELS];
    float chw[CHANNELS * HEIGHT * WIDTH];

    ssize_t total = 0;
    ssize_t n;

    fd = open("/dev/tile", O_RDONLY);
    if (fd < 0) {
        perror("open /dev/tile");
        return 1;
    }

    while (total < TILE_BYTES) {
        n = read(fd, (char *)hwc + total, TILE_BYTES - total);

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

    if (total != TILE_BYTES) {
        fprintf(stderr,
                "Expected %d bytes, received %zd bytes\n",
                TILE_BYTES, total);
        return 1;
    }

    /*
     * Input layout:
     *   hwc[(row * WIDTH + col) * CHANNELS + band]
     *
     * Output layout:
     *   chw[(band * HEIGHT + row) * WIDTH + col]
     */
    for (int row = 0; row < HEIGHT; row++) {
        for (int col = 0; col < WIDTH; col++) {
            for (int band = 0; band < CHANNELS; band++) {
                size_t src =
                    ((size_t)row * WIDTH + col) * CHANNELS + band;

                size_t dst =
                    ((size_t)band * HEIGHT + row) * WIDTH + col;

                chw[dst] = (float)hwc[src] / 10000.0f;
            }
        }
    }

    printf("Read and converted %zd bytes\n", total);

    printf("First pixel, six bands:\n");
    for (int band = 0; band < CHANNELS; band++) {
        size_t index = (size_t)band * HEIGHT * WIDTH;
        printf("  band %d: raw=%u normalized=%.6f\n",
               band,
               hwc[band],
               chw[index]);
    }

    printf("Pixel (row=1, col=0), six bands:\n");
    for (int band = 0; band < CHANNELS; band++) {
        size_t raw_index = CHANNELS + band;
        size_t chw_index = (size_t)band * HEIGHT * WIDTH + WIDTH;

        printf("  band %d: raw=%u normalized=%.6f\n",
               band,
               hwc[raw_index],
               chw[chw_index]);
    }

    return 0;
}
