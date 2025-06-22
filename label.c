#include "label.h"
#include <stdio.h>
#include <stdlib.h>

int index_label = 0; // Global variable for label index

char *new_label() {
    char *buffer = malloc(32 * sizeof(char));
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed for new label\n");
        exit(EXIT_FAILURE);
    }
    snprintf(buffer, 32, "L%d", index_label++);
    return buffer;
}

void free_label(char *label) {
    free(label);
}