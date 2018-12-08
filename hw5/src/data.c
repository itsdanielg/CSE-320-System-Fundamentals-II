#include "data.h"
#include "debug.h"
#include <string.h>

/*
 * A "blob" consists of arbitrary data having a specified size
 * and content.  The content must not be null.  Once a blob has
 * been created, its size and content do not change.  A blob has
 * a reference count field, which is used to keep track of the
 * number of pointers that currently exist pointing to the blob.
 * New pointers are created using blob_ref(), which increments the
 * reference count.  Pointers are destroyed using blob_unref(),
 * which decrements the reference count.  As long as the reference
 * count of a blob is nonzero, it will not be freed.
 */
// typedef struct blob {
//     pthread_mutex_t mutex;     // Mutex to protect reference count
//     int refcnt;
//     size_t size;
//     char *content;
//     char *prefix;              // String prefix of content (for debugging)
// } BLOB;

/*
 * A key consists of a pointer to a blob and a hash of the blob data.
 */
// typedef struct key {
//     int hash;
//     BLOB *blob;
// } KEY;

/*
 * A VERSION represents a single version of a value associated with a key
 * in the transactional store.  Each version has a creator transaction,
 * a pointer to a blob and links to the next and previous versions in the
 * list of all versions in the same map entry.
 */
// typedef struct version {
//     TRANSACTION *creator;
//     BLOB *blob;
//     struct version *next;
//     struct version *prev;
// } VERSION;

/*
 * Create a blob with given content and size.
 * The content is copied, rather than shared with the caller.
 * The returned blob has one reference, which becomes the caller's
 * responsibility.
 *
 * @param content  The content of the blob.
 * @param size  The size in bytes of the content.
 * @return  The new blob, which has reference count 1.
 */
BLOB *blob_create(char *content, size_t size) {
    BLOB* blob = malloc(sizeof(BLOB));
    pthread_mutex_init(&blob->mutex, NULL);
    blob->refcnt = 1;
    blob->size = size;
    blob->content = content;
    blob->prefix = content + '\0';
    return blob;
}

/*
 * Increase the reference count on a blob.
 *
 * @param bp  The blob.
 * @param why  Short phrase explaining the purpose of the increase.
 * @return  The blob pointer passed as the argument.
 */
BLOB *blob_ref(BLOB *bp, char *why) {
    pthread_mutex_lock(&bp->mutex);
    bp->refcnt++;
    debug("Blob reference count increased: %s", why);
    pthread_mutex_unlock(&bp->mutex);
    return bp;
}

/*
 * Decrease the reference count on a blob.
 * If the reference count reaches zero, the blob is freed.
 *
 * @param bp  The blob.
 * @param why  Short phrase explaining the purpose of the decrease.
 */
void blob_unref(BLOB *bp, char *why) {
    pthread_mutex_lock(&bp->mutex);
    bp->refcnt--;
    debug("Blob reference count decreased: %s", why);
    if (bp->refcnt == 0) {
        free(bp);
        return;
    }
    pthread_mutex_unlock(&bp->mutex);
}

/*
 * Compare two blobs for equality of their content.
 *
 * @param bp1  The first blob.
 * @param bp2  The second blob.
 * @return 0 if the blobs have equal content, nonzero otherwise.
 */
int blob_compare(BLOB *bp1, BLOB *bp2) {
    return strcmp(bp1->content, bp2->content);
}
/*
 * Hash function for hashing the content of a blob.
 *
 * @param bp  The blob.
 * @return  Hash of the blob.
 */
int blob_hash(BLOB *bp) {
    int hash = 0;
    int i = 0;
    for (i = 0; i < strlen(bp->content); i++) {
        hash += bp->content[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

/*
 * Create a key from a blob.
 * The key inherits the caller's reference to the blob.
 *
 * @param bp  The blob.
 * @return  the newly created key.
 */
KEY *key_create(BLOB *bp) {
    KEY* key = malloc(sizeof(KEY));
    key->hash = blob_hash(bp);
    key->blob = bp;
    blob_ref(bp, "Creating key");
    return key;
}

/*
 * Dispose of a key, decreasing the reference count of the contained blob.
 * A key must be disposed of only once and must not be referred to again
 * after it has been disposed.
 *
 * @param kp  The key.
 */
void key_dispose(KEY *kp) {
    blob_unref(kp->blob, "Disposing key");
    free(kp);
}

/*
 * Compare two keys for equality.
 *
 * @param kp1  The first key.
 * @param kp2  The second key.
 * @return  0 if the keys are equal, otherwise nonzero.
 */
int key_compare(KEY *kp1, KEY *kp2) {
    if (kp1->hash != kp2->hash) {
        return -1;
    }
    return blob_compare(kp1->blob, kp2->blob);
}

/*
 * Create a version of a blob for a specified creator transaction.
 * The version inherits the caller's reference to the blob.
 * The reference count of the creator transaction is increased to
 * account for the reference that is stored in the version.
 *
 * @param tp  The creator transaction.
 * @param bp  The blob.
 * @return  The newly created version.
 */
VERSION *version_create(TRANSACTION *tp, BLOB *bp) {
    VERSION* version = malloc(sizeof(VERSION));
    version->creator = tp;
    trans_ref(tp, "Version created");
    version->blob = bp;
    blob_ref(bp, "Version created");
    version->next = NULL;   // RETURN LATER
    version->prev = NULL;   // RETURN LATER
    return version;
}

/*
 * Dispose of a version, decreasing the reference count of the
 * creator transaction and contained blob.  A version must be
 * disposed of only once and must not be referred to again once
 * it has been disposed.
 *
 * @param vp  The version to be disposed.
 */
void version_dispose(VERSION *vp) {
    trans_unref(vp->creator, "Version disposed");
    blob_unref(vp->blob, "Version disposed");
    vp->prev->next = vp->next;
    vp->next->prev = vp->prev;
    vp->prev = NULL;
    vp->next = NULL;
    free(vp);
}
