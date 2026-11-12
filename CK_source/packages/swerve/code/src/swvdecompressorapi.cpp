// swvdecompressorapi.cpp - streaming zlib decompressor of the Swerve porting layer.
// The engine core starts a decompression with a format tag, feeds compressed chunks and receives
// every inflated block through decompress_AcceptBytes. Only the 'ZLIB' format exists.
#include "zlib.h"
#include "SwvPort.h"

// ---- constants ----
/** Format tag of the only supported compression ('ZLIB' as a little-endian word). */
static const unsigned int DECOMPRESSOR_FORMAT_ZLIB = 0x5a4c4942;
/** Size of the inflate output block handed to the engine. */
static const unsigned int DECOMPRESSOR_BLOCK_SIZE = 0x400;

extern "C" {

/** Begin a decompression of `format`; stores the new state in `*handle`. Returns 1 on success. */
int swvDecompressOnDataStart(unsigned int format, void* userData, swvDecompressor** handle)
{
    *handle = NULL;
    if (format != DECOMPRESSOR_FORMAT_ZLIB) {
        return 0;
    }

    swvDecompressor* decompressor = (swvDecompressor*)swvSystemAllocate(sizeof(swvDecompressor));
    if (decompressor == NULL) {
        return 0;
    }
    *handle = decompressor;
    decompressor->m_format = DECOMPRESSOR_FORMAT_ZLIB;
    decompressor->m_userData = userData;
    swvMemSet(decompressor->m_block, 0, DECOMPRESSOR_BLOCK_SIZE);

    z_stream* stream = (z_stream*)swvSystemAllocate(sizeof(z_stream));
    decompressor->m_stream = stream;
    if (stream == NULL) {
        return 0;
    }
    stream->next_in = NULL;
    stream->avail_in = 0;
    stream->zalloc = NULL;
    stream->zfree = NULL;
    stream->opaque = NULL;
    if (inflateInit(stream) != Z_OK) {
        return 0;
    }
    return 1;
}

/** Inflate one compressed chunk, handing every output block to the engine. Returns 1 on success. */
int swvDecompressOnData(const unsigned char* data, unsigned int size, swvDecompressor** handle)
{
    if (handle == NULL) {
        return 0;
    }
    swvDecompressor* decompressor = *handle;
    if (decompressor == NULL) {
        return 0;
    }
    if (decompressor->m_format != DECOMPRESSOR_FORMAT_ZLIB) {
        return 0;
    }

    z_stream* stream = decompressor->m_stream;
    int keepGoing = 1;
    int status;
    stream->next_in = (Bytef*)data;
    stream->avail_in = size;
    do {
        stream->next_out = decompressor->m_block;
        stream->avail_out = DECOMPRESSOR_BLOCK_SIZE;
        status = inflate(stream, Z_SYNC_FLUSH);
        if (stream->avail_out != DECOMPRESSOR_BLOCK_SIZE) {
            keepGoing = decompress_AcceptBytes(decompressor->m_userData, DECOMPRESSOR_BLOCK_SIZE - stream->avail_out,
                                               decompressor->m_block);
        }
    } while (keepGoing && status == Z_OK && stream->avail_out == 0);

    if (!keepGoing) {
        return 0;
    }
    if (status == Z_OK || status == Z_STREAM_END) {
        return 1;
    }
    // Z_BUF_ERROR without any output only means the chunk was used up.
    int usedUp = (status == Z_BUF_ERROR && stream->avail_out == DECOMPRESSOR_BLOCK_SIZE);
    return usedUp;
}

/** Finish the decompression and free its state; returns 1 when zlib ended cleanly. */
int swvDecompressOnDataEnd(swvDecompressor** handle)
{
    int result = 0;
    if (handle == NULL) {
        return 0;
    }
    swvDecompressor* decompressor = *handle;
    if (decompressor == NULL) {
        return 0;
    }
    if (decompressor->m_format == DECOMPRESSOR_FORMAT_ZLIB) {
        int status = inflateEnd(decompressor->m_stream);
        swvSystemFree(decompressor->m_stream);
        result = (status == Z_OK);
    }
    swvSystemFree(decompressor);
    *handle = NULL;
    return result;
}

/** Abort a decompression after an error (same cleanup as swvDecompressOnDataEnd). */
void swvDecompressError(swvDecompressor** handle)
{
    swvDecompressOnDataEnd(handle);
}

/** zlib Adler-32 checksum. */
unsigned long swvAdler32(unsigned long adler, const unsigned char* data, unsigned int size)
{
    return adler32(adler, data, size);
}

}
