/*
 * Optimized memcpy from NV12t GPU buffer to system buffer
 *
 * Address 16 alignment required
 *
 * Detail:
 *      https://software.intel.com/en-us/articles/copying-accelerated-video-decode-frame-buffers
 *      https://software.intel.com/en-us/articles/increasing-memory-throughput-with-intel-streaming-simd-extensions-4-intel-sse4-streaming-load
 */
void *memcpy_from_uswc_sse4(void *dst, void *src, size_t size);
