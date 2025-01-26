#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#define HASH_TABLE_SIZE 1024

typedef struct Node {
    char* word;
    int count;
    struct Node* next;
} Node;

unsigned int hash(const char* str) {
    unsigned int hash = 0;
    while (*str) {
        hash = (hash * 31) + tolower(*str++);
    }
    return hash % HASH_TABLE_SIZE;
}

void insert(Node** table, const char* word) {
    unsigned int index = hash(word);
    Node* current = table[index];
    while (current) {
        if (strcmp(current->word, word) == 0) {
            current->count++;
            return;
        }
        current = current->next;
    }
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->word = strdup(word);
    new_node->count = 1;
    new_node->next = table[index];
    table[index] = new_node;
}

void write_table(Node** table, FILE* out) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        Node* current = table[i];
        while (current) {
            fwrite(&current->count, sizeof(int), 1, out);
            int word_length = strlen(current->word);
            fwrite(&word_length, sizeof(int), 1, out);
            fwrite(current->word, sizeof(char), word_length, out);
            current = current->next;
        }
    }
}

void free_table(Node** table) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        Node* current = table[i];
        while (current) {
            Node* temp = current;
            current = current->next;
            free(temp->word);
            free(temp);
        }
    }
}

void compress(const char* input_file, const char* output_file) {
    Node* hash_table[HASH_TABLE_SIZE] = {0};

    FILE* in = fopen(input_file, "r");
    if (in == NULL) {
        fprintf(stderr, "Failed to open file %s for reading\n", input_file);
        return;
    }

    FILE* out = fopen(output_file, "wb");
    if (out == NULL) {
        fprintf(stderr, "Failed to open file %s for writing\n", output_file);
        fclose(in);
        return;
    }

    char* line = (char*)malloc(1024 * sizeof(char));
    if (line == NULL) {
        fprintf(stderr, "Memory allocation failed for line\n");
        fclose(in);
        fclose(out);
        return;
    }

    while (fgets(line, 1024, in) != NULL) {
        char* token = strtok(line, " \t\n\r");
        while (token != NULL) {
            insert(hash_table, token);
            token = strtok(NULL, " \t\n\r");
        }
    }

    write_table(hash_table, out);
    free_table(hash_table);
    free(line);
    fclose(in);
    fclose(out);
}

void uncompress(const char* input_file, const char* output_file) {
    FILE* in = fopen(input_file, "rb");
    if (in == NULL) {
        fprintf(stderr, "Failed to open file %s for reading\n", input_file);
        return;
    }

    FILE* out = fopen(output_file, "w");
    if (out == NULL) {
        fprintf(stderr, "Failed to open file %s for writing\n", output_file);
        fclose(in);
        return;
    }

    int count;
    int word_length;
    char* word = NULL;

    while (fread(&count, sizeof(int), 1, in) == 1) {
        if (fread(&word_length, sizeof(int), 1, in) != 1) {
            fprintf(stderr, "Error reading word length from %s\n", input_file);
            break;
        }

        word = (char*)malloc((word_length + 1) * sizeof(char));
        if (word == NULL) {
            fprintf(stderr, "Memory allocation failed for word\n");
            break;
        }

        if (fread(word, sizeof(char), word_length, in) != word_length) {
            fprintf(stderr, "Error reading word from %s\n", input_file);
            free(word);
            break;
        }

        word[word_length] = '\0';

        for (int i = 0; i < count; i++) {
            fprintf(out, "%s ", word);
        }

        free(word);
    }

    fclose(in);
    fclose(out);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s -c <input_files...> | -d <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-c") == 0) {
        for (int i = 2; i < argc; i++) {
            char* output_name = (char*)malloc(strlen(argv[i]) + 6);
            if (!output_name) {
                fprintf(stderr, "Memory allocation failed for output file name\n");
                return 1;
            }

            strcpy(output_name, argv[i]);
            strtok(output_name, ".");   // remove the extension
            strcat(output_name, ".micu");
            compress(argv[i], output_name);
            free(output_name);
        }
    } else if (strcmp(argv[1], "-d") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage for decompression: %s -d <input_file> <output_file>\n", argv[0]);
            return 1;
        }
        uncompress(argv[2], argv[3]);
    } else {
        fprintf(stderr, "Invalid option: %s\n", argv[1]);
        fprintf(stderr, "Usage: %s -c <input_files...> | -d <input_file> <output_file>\n", argv[0]);
        return 1;
    }

    return 0;
}
