#ifndef _GUESSER_H
#define _GUESSER_H

#include <stdint.h>
#include <stdbool.h>

// Power of two
#define QUEUE_SIZE 32

typedef struct {
    uint8_t guess[FAT_DENTRY_SIZE];
    uint32_t measurement;
} queue_entry;

extern queue_entry queue[QUEUE_SIZE];
extern volatile uint32_t qptr_write_guess;         // Advance when a guess is enqueued
extern volatile uint32_t qptr_read_guess;          // Follows when guess is sent
extern volatile uint32_t qptr_write_measurement;   // Follows when timestamp result is measured
extern volatile uint32_t qptr_read_measurement;    // Follows when result is dequeued

static inline queue_entry* qentry(uint32_t i) {
    return &queue[i % QUEUE_SIZE];
}

void reset_pulse(void);
void mainloop_poll(void);

void guess_enqueue(void);
void guess_filename(const char *name, const char *ext);

#endif // _GUESSER_H
