#include "transaction.h"
#include <stdlib.h>
#include "debug.h"

/*
 * A transaction is a context within which to perform a sequence of operations
 * on a transactional store.  Every transaction has a unique transaction ID.
 * There is an ordering on transaction IDs, so that it makes sense to say that
 * one transaction ID is "less than" another.
 *
 * When it is first created, a transaction is in the "pending" state.
 * Each time an operation is performed in the context of a transaction,
 * there is a possibility that the transaction will enter the "aborted" state,
 * due to a conflict with other transactions executing concurrently with it.
 * Once a transaction has aborted, it is not possible to perform any further operations
 * within it and all effects that it has had on the store are deleted.  Once all the
 * desired operations have been performed within a transaction, an attempt can be made
 * to "commit" the transaction.  The transaction will then either abort or enter
 * the "committed" state.  Once a transaction has entered the committed state,
 * all effects that it has had on the store are made permanent.
 */

/*
 * Transaction status.
 * Also used as return values from transaction operations.
 */
// typedef enum { TRANS_PENDING, TRANS_COMMITTED, TRANS_ABORTED } TRANS_STATUS;

/*
 * A transaction may become "dependent" on another transaction.
 * This occurs when a transaction accesses an entry in the store that has
 * previously been accessed by a pending transaction having a smaller transaction
 * ID.  Once that occurs, the dependent transaction cannot commit until the
 * the transaction on which it depends has committed.  Moreover, if the
 * other transaction aborts, then the dependent transaction must also abort.
 *
 * The dependencies of a transaction are recorded in a "dependency set",
 * which is part of the representation of a transaction.  A dependency set
 * is represented as a singly linked list of "dependency" nodes having the
 * following structure.  Any given transaction can occur at most once in a
 * single dependency set.
 */
// typedef struct dependency {
//   struct transaction *trans;  // Transaction on which the dependency depends.
//   struct dependency *next;    // Next dependency in the set.
// } DEPENDENCY;

/*
 * Structure representing a transaction.
 */
// typedef struct transaction {
//   unsigned int id;           // Transaction ID.
//   unsigned int refcnt;       // Number of references (pointers) to transaction.
//   TRANS_STATUS status;       // Current transaction status.
//   DEPENDENCY *depends;       // Singly-linked list of dependencies.
//   int waitcnt;               // Number of transactions waiting for this one.
//   sem_t sem;                 // Semaphore to wait for transaction to commit or abort.
//   pthread_mutex_t mutex;     // Mutex to protect fields.
//   struct transaction *next;  // Next in list of all transactions
//   struct transaction *prev;  // Prev in list of all transactions.
// } TRANSACTION;

/*
 * For debugging purposes, all transactions having a nonzero reference count
 * are maintained in a circular, doubly linked list which has the following
 * sentinel as its head.
 */
// TRANSACTION trans_list;
int transCount;
int transID;

/*
 * Initialize the transaction manager.
 */
void trans_init(void) {
    pthread_mutex_init(&trans_list.mutex, NULL);
    trans_list.next = &trans_list;
    trans_list.prev = &trans_list;
    transCount = 0;
    transID = 0;
}

/*
 * Finalize the transaction manager.
 */
void trans_fini(void) {
    TRANSACTION* currentTransaction = trans_list.next;
    while (currentTransaction != &trans_list) {
        TRANSACTION* nextTransaction = currentTransaction->next;
        free(currentTransaction);
        currentTransaction = nextTransaction;
    }
}
/*
 * Create a new transaction.
 *
 * @return  A pointer to the new transaction (with reference count 1)
 * is returned if creation is successful, otherwise NULL is returned.
 */
TRANSACTION *trans_create(void) {
    transCount++;
    transID++;
    TRANSACTION* transaction = malloc(sizeof(TRANSACTION));
    transaction->id = transID;
    transaction->refcnt = 1;
    transaction->status = TRANS_PENDING;
    transaction->depends = NULL;
    transaction->waitcnt = 0;
    if (sem_init(&transaction->sem, 0, 1) != 0) {
        transCount--;
        transID--;
        free(transaction);
        return NULL;
    }
    if (pthread_mutex_init(&transaction->mutex, NULL) != 0) {
        transCount--;
        transID--;
        free(transaction);
        return NULL;
    }
    if (transCount == 1) {
        transaction->next = &trans_list;
        transaction->prev = &trans_list;;
        trans_list.next = transaction;
        trans_list.prev = transaction;
    }
    else {
        transaction->next = &trans_list;
        transaction->prev = trans_list.prev;
        trans_list.prev->next = transaction;
        trans_list.prev = transaction;
    }
    return transaction;
}

/*
 * Increase the reference count on a transaction.
 *
 * @param tp  The transaction.
 * @param why  Short phrase explaining the purpose of the increase.
 * @return  The transaction pointer passed as the argument.
 */
TRANSACTION *trans_ref(TRANSACTION *tp, char *why) {
    pthread_mutex_lock(&tp->mutex);
    tp->refcnt++;
    debug("Transaction reference count increased: %s", why);
    pthread_mutex_unlock(&tp->mutex);
    return tp;
}

/*
 * Decrease the reference count on a transaction.
 * If the reference count reaches zero, the transaction is freed.
 *
 * @param tp  The transaction.
 * @param why  Short phrase explaining the purpose of the decrease.
 */
void trans_unref(TRANSACTION *tp, char *why) {
    pthread_mutex_lock(&tp->mutex);
    tp->refcnt--;
    debug("Transaction reference count decreased: %s", why);
    if (tp->refcnt == 0) {
        tp->prev->next = tp->next;
        tp->next->prev = tp->prev;
        tp->next = NULL;
        tp->prev = NULL;
        free(tp);
        transCount--;
        return;
    }
    pthread_mutex_unlock(&tp->mutex);
}

/*
 * Add a transaction to the dependency set for this transaction.
 *
 * @param tp  The transaction to which the dependency is being added.
 * @param dtp  The transaction that is being added to the dependency set.
 */
void trans_add_dependency(TRANSACTION *tp, TRANSACTION *dtp) {
    pthread_mutex_lock(&tp->mutex);
    DEPENDENCY* dependency = malloc(sizeof(DEPENDENCY));
    dependency->trans = dtp;
    dependency->next = NULL;
    if (tp->depends == NULL) {
        tp->depends = dependency;
    }
    else {
        char alreadyDependent = 0;
        DEPENDENCY* currentDependency = tp->depends;
        while (currentDependency != NULL) {
            if (currentDependency->trans == dtp) {
                alreadyDependent = 1;
                break;
            }
            if (currentDependency->next == NULL) {
                currentDependency->next = dependency;
                dtp->refcnt++;
                break;
            }
            else {
                currentDependency = currentDependency->next;
            }
        }
        if (alreadyDependent) {
            free(dependency);
        }
    }
    pthread_mutex_unlock(&tp->mutex);
}

/*
 * Try to commit a transaction.  Committing a transaction requires waiting
 * for all transactions in its dependency set to either commit or abort.
 * If any transaction in the dependency set abort, then the dependent
 * transaction must also abort.  If all transactions in the dependency set
 * commit, then the dependent transaction may also commit.
 *
 * In all cases, this function consumes a single reference to the transaction
 * object.
 *
 * @param tp  The transaction to be committed.
 * @return  The final status of the transaction: either TRANS_ABORTED,
 * or TRANS_COMMITTED.
 */
TRANS_STATUS trans_commit(TRANSACTION *tp) {
    pthread_mutex_lock(&tp->mutex);
    if (tp->depends != NULL) {
        DEPENDENCY* currentDependency = tp->depends;
        while (currentDependency != NULL) {
            sem_wait(&currentDependency->trans->sem);
            currentDependency = currentDependency->next;
        }
    }
    while (tp->waitcnt > 0) {
        sem_post(&tp->sem);
        tp->waitcnt--;
    }
    tp->status = TRANS_COMMITTED;
    if (tp->depends != NULL) {
        DEPENDENCY* currentDependency = tp->depends;
        while (currentDependency != NULL) {
            TRANS_STATUS status = trans_get_status(currentDependency->trans);
            if (status == TRANS_ABORTED) {
                tp->status = TRANS_ABORTED;
                break;
            }
            currentDependency = currentDependency->next;
        }
    }
    TRANS_STATUS status = tp->status;
    pthread_mutex_unlock(&tp->mutex);
    if (status == TRANS_COMMITTED) {
        trans_unref(tp, "Transaction Committed");
    }
    else {
        trans_unref(tp, "Transaction Aborted");
    }
    return status;
}

/*
 * Abort a transaction.  If the transaction has already committed, it is
 * a fatal error and the program crashes.  If the transaction has already
 * aborted, no change is made to its state.  If the transaction is pending,
 * then it is set to the aborted state, and any transactions dependent on
 * this transaction must also abort.
 *
 * In all cases, this function consumes a single reference to the transaction
 * object.
 *
 * @param tp  The transaction to be aborted.
 * @return  TRANS_ABORTED.
 */
TRANS_STATUS trans_abort(TRANSACTION *tp) {
    pthread_mutex_lock(&tp->mutex);
    if (tp->status == TRANS_COMMITTED) {
        pthread_mutex_unlock(&tp->mutex);
        return -1;
    }
    else if (tp->status == TRANS_PENDING) {
        while (tp->waitcnt > 0) {
            sem_post(&tp->sem);
            tp->waitcnt--;
        }
        tp->status = TRANS_ABORTED;
    }
    pthread_mutex_unlock(&tp->mutex);
    trans_unref(tp, "Transaction Aborted");
    return TRANS_ABORTED;
}

/*
 * Get the current status of a transaction.
 * If the value returned is TRANS_PENDING, then we learn nothing,
 * because unless we are holding the transaction mutex the transaction
 * could be aborted at any time.  However, if the value returned is
 * either TRANS_COMMITTED or TRANS_ABORTED, then that value is the
 * stable final status of the transaction.
 *
 * @param tp  The transaction.
 * @return  The status of the transaction, as it was at the time of call.
 */
TRANS_STATUS trans_get_status(TRANSACTION *tp) {
    return tp->status;
}

/*
 * Print information about a transaction to stderr.
 * No locking is performed, so this is not thread-safe.
 * This should only be used for debugging.
 *
 * @param tp  The transaction to be shown.
 */
void trans_show(TRANSACTION *tp) {
    fprintf(stderr, "\nTransaction ID: %u\n", tp->id);
    fprintf(stderr, "Reference Count: %u\n", tp->refcnt);
    fprintf(stderr, "Transaction Status: %d\n", (int)tp->status);
    fprintf(stderr, "First Dependency: %p\n", tp->depends);
    fprintf(stderr, "Waiting Count: %d\n", tp->waitcnt);
    fprintf(stderr, "Next Transaction: %p\n", tp->next);
    fprintf(stderr, "Previous Transaction: %p\n", tp->prev);
}

/*
 * Print information about all transactions to stderr.
 * No locking is performed, so this is not thread-safe.
 * This should only be used for debugging.
 */
void trans_show_all(void) {
    TRANSACTION* currentTransaction = trans_list.next;
    while (currentTransaction != &trans_list) {
        trans_show(currentTransaction);
        currentTransaction = currentTransaction->next;
    }
}