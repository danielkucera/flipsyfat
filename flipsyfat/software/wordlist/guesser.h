#ifndef _GUESSER_H
#define _GUESSER_H

#include <stdint.h>
#include <stdbool.h>

// Power of two
#define QUEUE_SIZE 128

typedef struct {
    uint8_t guess[FAT_DENTRY_SIZE];
    uint32_t measurement;
    uint32_t replicate_count;
} queue_entry;

// Uniquely track each experiment. Actual queue position is modulo QUEUE_SIZE
typedef uint64_t qptr_t;

extern queue_entry queue[QUEUE_SIZE];
extern volatile qptr_t qptr_write_guess;         // Advance when a guess is enqueued
extern volatile qptr_t qptr_read_guess;          // Follows when guess is sent
extern volatile qptr_t qptr_write_measurement;   // Follows when timestamp result is measured
extern volatile qptr_t qptr_read_measurement;    // Follows when result is dequeued

static inline queue_entry* qentry(qptr_t i) {
    return &queue[i % QUEUE_SIZE];
}

void reset_pulse(void);
void mainloop_poll(void);

void guess_dentry(const uint8_t *dentry);
void guess_filename(const char *name, const char *ext);

#endif // _GUESSER_H
