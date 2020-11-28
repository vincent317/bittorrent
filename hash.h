#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include <stddef.h>

struct sha1sum_ctx;

/* This takes an initial salt and salt length and returns a context that can
 * be used with the other functions. If len is 0, salt can be NULL. Returns
 * NULL on error */
struct sha1sum_ctx * sha1sum_create(const uint8_t *salt, size_t len);

/* With a valid context, add the payload to the hash. Repeated calls of
 * update will let you compute the hash incrementally. Function returns 0 on
 * success.
 */
int sha1sum_update(struct sha1sum_ctx*, const uint8_t *payload, size_t len);

/* With a valid context, add the payload (with a specified length) to the
 * current hash and output the full checksum into out. out must have enough
 * space to write 20 bytes of output. Function returns 0 on success. After
 * this call, the context is no longer in a valid state and must be either
 * reset or destroyed.
 */
int sha1sum_finish(struct sha1sum_ctx*, const uint8_t *payload, size_t len, uint8_t *out);

/* Reset the context to prepare it to computer another hash. This reuses the
 * originally given salt. This is equivalent (but more efficient than)
 * destroying and recreating the context for each hash that you want to
 * compute. Returns 0 on success */
int sha1sum_reset(struct sha1sum_ctx*);

/* Destroy the context. This frees all memory and resources associated with
 * the context */
int sha1sum_destroy(struct sha1sum_ctx*);

#endif

