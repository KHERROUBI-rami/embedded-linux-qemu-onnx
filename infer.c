#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "onnxruntime_c_api.h"

#define WIDTH 64
#define HEIGHT 64
#define CHANNELS 6
#define CLASSES 10

static const char *class_names[CLASSES] = {
    "AnnualCrop",
    "Forest",
    "HerbaceousVegetation",
    "Highway",
    "Industrial",
    "Pasture",
    "PermanentCrop",
    "Residential",
    "River",
    "SeaLake"
};

static void fail_status(
    const OrtApi *api,
    OrtStatus *status,
    const char *where)
{
    if (status != NULL) {
        fprintf(stderr, "%s: %s\n", where, api->GetErrorMessage(status));
        api->ReleaseStatus(status);
    } else {
        fprintf(stderr, "%s\n", where);
    }

    exit(EXIT_FAILURE);
}

#define ORT_CHECK(api, expression)                         \
    do {                                                    \
        OrtStatus *status__ = (expression);                 \
        if (status__ != NULL)                               \
            fail_status((api), status__, #expression);      \
    } while (0)

static int read_full(int fd, void *buffer, size_t size)
{
    size_t total = 0;
    char *ptr = (char *)buffer;

    while (total < size) {
        ssize_t count = read(fd, ptr + total, size - total);

        if (count < 0) {
            if (errno == EINTR)
                continue;

            perror("read /dev/tile");
            return -1;
        }

        if (count == 0)
            break;

        total += (size_t)count;
    }

    return total == size ? 0 : -1;
}

static void softmax(
    const float *logits,
    float *probabilities,
    size_t count)
{
    float maximum = logits[0];
    float sum = 0.0f;

    for (size_t i = 1; i < count; i++) {
        if (logits[i] > maximum)
            maximum = logits[i];
    }

    for (size_t i = 0; i < count; i++) {
        probabilities[i] = expf(logits[i] - maximum);
        sum += probabilities[i];
    }

    for (size_t i = 0; i < count; i++)
        probabilities[i] /= sum;
}

int main(void)
{
    const OrtApi *api =
        OrtGetApiBase()->GetApi(ORT_API_VERSION);

    if (api == NULL) {
        fprintf(stderr, "Could not obtain ONNX Runtime API\n");
        return EXIT_FAILURE;
    }

    int tile_fd = open("/dev/tile", O_RDONLY);
    if (tile_fd < 0) {
        perror("open /dev/tile");
        return EXIT_FAILURE;
    }

    /*
     * The tile is stored as HWC uint16:
     *
     *   [row][column][band]
     *
     * Dimensions:
     *
     *   64 x 64 x 6
     */
    uint16_t hwc[WIDTH * HEIGHT * CHANNELS];

    /*
     * ONNX Runtime input is NCHW float32:
     *
     *   [batch][channel][row][column]
     *
     * Shape:
     *
     *   [1, 6, 64, 64]
     */
    float input_data[CHANNELS * HEIGHT * WIDTH];

    if (read_full(tile_fd, hwc, sizeof(hwc)) != 0) {
        fprintf(
            stderr,
            "Could not read %zu bytes from /dev/tile\n",
            sizeof(hwc));

        close(tile_fd);
        return EXIT_FAILURE;
    }

    close(tile_fd);

    /*
     * Convert HWC uint16 to CHW float32.
     * This matches dataset.py:
     *
     *   tensor = tensor / 10000.0
     */
    for (int row = 0; row < HEIGHT; row++) {
        for (int col = 0; col < WIDTH; col++) {
            for (int band = 0; band < CHANNELS; band++) {
                size_t source_index =
                    ((size_t)row * WIDTH + col) * CHANNELS + band;

                size_t destination_index =
                    (size_t)band * HEIGHT * WIDTH +
                    (size_t)row * WIDTH +
                    col;

                input_data[destination_index] =
                    (float)hwc[source_index] / 10000.0f;
            }
        }
    }

    OrtEnv *env = NULL;
    OrtSessionOptions *session_options = NULL;
    OrtSession *session = NULL;
    OrtMemoryInfo *memory_info = NULL;
    OrtValue *input_tensor = NULL;
    OrtValue *output_tensor = NULL;
    OrtAllocator *allocator = NULL;

    ORT_CHECK(
        api,
        api->CreateEnv(
            ORT_LOGGING_LEVEL_WARNING,
            "arm-lab",
            &env));

    ORT_CHECK(
        api,
        api->CreateSessionOptions(
            &session_options));

    ORT_CHECK(
        api,
        api->SetIntraOpNumThreads(
            session_options,
            1));

    ORT_CHECK(
        api,
        api->SetInterOpNumThreads(
            session_options,
            1));

    ORT_CHECK(
        api,
        api->CreateSession(
            env,
            "/models/model_int8_static.onnx",
            session_options,
            &session));

    ORT_CHECK(
        api,
        api->CreateCpuMemoryInfo(
            OrtArenaAllocator,
            OrtMemTypeDefault,
            &memory_info));

    int64_t input_shape[4] = {
        1,
        CHANNELS,
        HEIGHT,
        WIDTH
    };

    ORT_CHECK(
        api,
        api->CreateTensorWithDataAsOrtValue(
            memory_info,
            input_data,
            sizeof(input_data),
            input_shape,
            4,
            ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
            &input_tensor));

    ORT_CHECK(
        api,
        api->GetAllocatorWithDefaultOptions(
            &allocator));

    char *input_name = NULL;
    char *output_name = NULL;

    ORT_CHECK(
        api,
        api->SessionGetInputName(
            session,
            0,
            allocator,
            &input_name));

    ORT_CHECK(
        api,
        api->SessionGetOutputName(
            session,
            0,
            allocator,
            &output_name));

    /*
     * Run expects arrays of:
     *
     *   const char * const *
     */
    const char *input_names[] = {
        input_name
    };

    const char *output_names[] = {
        output_name
    };

    const OrtValue *inputs[] = {
        input_tensor
    };

    OrtValue *outputs[] = {
        NULL
    };

    ORT_CHECK(
        api,
        api->Run(
            session,
            NULL,
            input_names,
            inputs,
            1,
            output_names,
            1,
            outputs));

    output_tensor = outputs[0];

    float *logits = NULL;

    ORT_CHECK(
        api,
        api->GetTensorMutableData(
            output_tensor,
            (void **)&logits));

    float probabilities[CLASSES];

    softmax(
        logits,
        probabilities,
        CLASSES);

    int best_index = 0;

    for (int i = 1; i < CLASSES; i++) {
        if (probabilities[i] > probabilities[best_index])
            best_index = i;
    }

    printf(
        "Prediction: %s (%.2f%%)\n",
        class_names[best_index],
        probabilities[best_index] * 100.0f);

    printf("All probabilities:\n");

    for (int i = 0; i < CLASSES; i++) {
        printf(
            "  %-24s %.2f%%\n",
            class_names[i],
            probabilities[i] * 100.0f);
    }

    /*
     * Free the input/output name strings.
     * AllocatorFree returns an OrtStatus in this API version,
     * so its return value must be checked.
     */
    ORT_CHECK(
        api,
        api->AllocatorFree(
            allocator,
            input_name));

    ORT_CHECK(
        api,
        api->AllocatorFree(
            allocator,
            output_name));

    api->ReleaseValue(output_tensor);
    api->ReleaseValue(input_tensor);
    api->ReleaseMemoryInfo(memory_info);
    api->ReleaseSession(session);
    api->ReleaseSessionOptions(session_options);
    api->ReleaseEnv(env);

    return EXIT_SUCCESS;
}
