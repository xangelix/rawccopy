#include "rawccopy_api.h"
#include "context.h"
#include "path.h"
#include "mft.h"
#include "index.h"
#include "attribs.h"
#include "byte-buffer.h"
#include <stdlib.h>
#include <string.h>

// The internal, private structure for our stream handle.
struct _rawccopy_stream {
    execution_context context;
    mft_file file;
    attribute_reader reader;
    uint64_t total_size;
};

rawccopy_stream* rawccopy_open(int argc, char* argv[]) {
    // 1. Perform initial setup using existing functions
    execution_context context = SetupContext(argc, argv);
    if (!context) return NULL;

    // 2. Find the target file's MFT reference (logic adapted from processor.c)
    uint64_t mft_ref = 0;
    bool found = false;
    if (context->parameters->mft_ref) {
        mft_ref = *context->parameters->mft_ref;
        found = true;
    } else {
        resolved_path res_path = NULL;
        if (TryParsePath(context, BaseString(context->parameters->source_path), &res_path)) {
            path_step hit = utarray_back(res_path);
            if (hit) {
                mft_ref = IndexEntryPtr(DerefStep(hit))->mft_reference;
                found = true;
            }
            DeletePath(res_path);
        }
    }

    if (!found) {
        CleanUp(context);
        return NULL;
    }

    // 3. Load the MFT record and find the default $DATA attribute
    mft_file file = LoadMFTFile(context, mft_ref);
    if (!file) {
        CleanUp(context);
        return NULL;
    }

    attribute data_attr = FirstAttribute(context, file, AttrTypeFlag(ATTR_DATA));
    // This could be extended to find named streams (ADS) if needed
    if (!data_attr || data_attr->name_len != 0) {
        DeleteMFTFile(file);
        CleanUp(context);
        return NULL;
    }

    // 4. Create an attribute_reader to manage the streaming state
    attribute_reader reader = OpenAttributeReader(context, file, data_attr);
    if (!reader) {
        DeleteMFTFile(file);
        CleanUp(context);
        return NULL;
    }

    // 5. Allocate and populate our stream handle struct
    rawccopy_stream* stream = (rawccopy_stream*)malloc(sizeof(rawccopy_stream));
    if (!stream) {
        CloseAttributeReader(reader);
        DeleteMFTFile(file);
        CleanUp(context);
        return NULL;
    }

    stream->context = context;
    stream->file = file;
    stream->reader = reader;
    stream->total_size = AttributeSize(data_attr);

    return stream;
}

int64_t rawccopy_read(rawccopy_stream* stream, uint8_t* buffer, uint64_t buffer_len) {
    // ---- START: ADD THIS BLOCK ----
    fprintf(stderr, "[DEBUG] ==> ENTERING rawccopy_read()\n");
    if (stream) {
        fprintf(stderr, "[DEBUG]     Stream Ptr: %p, Reader Position: %llu, Total Size: %llu\n",
                (void*)stream, AttributeReaderPosition(stream->reader), stream->total_size);
    } else {
        fprintf(stderr, "[DEBUG]     Stream Ptr is NULL!\n");
    }
    fprintf(stderr, "[DEBUG]     Buffer Ptr: %p, Buffer Len: %llu\n", (void*)buffer, buffer_len);
    // ---- END: ADD THIS BLOCK ----

    if (!stream || !buffer || AttributeReaderPosition(stream->reader) >= stream->total_size) {
        fprintf(stderr, "[DEBUG] <== EXITING rawccopy_read() with 0 (EOF or invalid args)\n"); // ADD THIS LINE
        return 0;
    }

    uint64_t bytes_to_read = buffer_len;
    uint64_t bytes_remaining = stream->total_size - AttributeReaderPosition(stream->reader);
    if (bytes_to_read > bytes_remaining) {
        bytes_to_read = bytes_remaining;
    }
    if (bytes_to_read == 0) {
        fprintf(stderr, "[DEBUG] <== EXITING rawccopy_read() with 0 (no bytes to read)\n"); // ADD THIS LINE
        return 0;
    }
    
    // ---- START: ADD THIS BLOCK ----
    fprintf(stderr, "[DEBUG]     Attempting to read %llu bytes via GetBytesFromAttribRdr()...\n", bytes_to_read);
    bytes read_data = GetBytesFromAttribRdr(stream->context, stream->reader, -1, bytes_to_read);
    fprintf(stderr, "[DEBUG]     ...GetBytesFromAttribRdr() returned.\n");
    fprintf(stderr, "[DEBUG]     read_data ptr: %p\n", (void*)read_data);
    // ---- END: ADD THIS BLOCK ----


    if (!read_data) {
        fprintf(stderr, "[DEBUG] <== EXITING rawccopy_read() with -1 (read error)\n"); // ADD THIS LINE
        return -1;
    }

    uint64_t bytes_read = read_data->buffer_len;
    if (bytes_read > 0) {
        memcpy(buffer, read_data->buffer, bytes_read);
    }
    
    DeleteBytes(read_data);

    // ---- START: ADD THIS LINE before returning ----
    fprintf(stderr, "[DEBUG] <== EXITING rawccopy_read() with %lld bytes read\n", (long long)bytes_read);
    // ---- END: ADD THIS LINE ----
    return (int64_t)bytes_read;
}

uint64_t rawccopy_size(rawccopy_stream* stream) {
    return stream ? stream->total_size : 0;
}

void rawccopy_close(rawccopy_stream* stream) {
    if (!stream) return;
    
    // Clean up all resources in the correct order
    CloseAttributeReader(stream->reader);
    DeleteMFTFile(stream->file);
    CleanUp(stream->context);
    free(stream);
}