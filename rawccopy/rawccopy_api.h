#ifndef RAWCOPY_API_H
#define RAWCOPY_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque pointer to the internal stream structure. Rust will only see this as a void pointer.
typedef struct _rawccopy_stream rawccopy_stream;

/**
 * @brief Initializes the library and prepares a file for reading based on command-line style arguments.
 * @param argc The number of arguments.
 * @param argv An array of argument strings.
 * @return A handle to the stream for use in other API calls, or NULL on error.
 */
rawccopy_stream* rawccopy_open(int argc, char* argv[]);

/**
 * @brief Reads data from the file stream into a caller-provided buffer.
 * @param stream The stream handle returned by rawccopy_open.
 * @param buffer A pointer to the destination buffer.
 * @param buffer_len The maximum number of bytes to read into the buffer.
 * @return The number of bytes actually read (can be less than buffer_len).
 * Returns 0 on End-of-File.
 * Returns -1 on error.
 */
int64_t rawccopy_read(rawccopy_stream* stream, uint8_t* buffer, uint64_t buffer_len);

/**
 * @brief Gets the total size of the file stream in bytes.
 * @param stream The stream handle.
 * @return The total size of the file's data attribute.
 */
uint64_t rawccopy_size(rawccopy_stream* stream);

/**
 * @brief Closes the stream and releases all associated C-side resources.
 * @param stream The stream handle to close.
 */
void rawccopy_close(rawccopy_stream* stream);

#ifdef __cplusplus
}
#endif

#endif // RAWCOPY_API_H