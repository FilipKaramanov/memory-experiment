/*
 * Assignment 4 – Linked List Memory Experiment (C)
 * Compile: gcc -Wall -Wextra -o FilipC FilipC.c
 * Run    : ./FilipC
 *
 * Reads Council_Member_Expenses CSV into a singly-linked list.
 * Each node holds a Row (council-member name + expense amount).
 * After loading, prints VmSize from /proc/self/status.
 * Also runs self-tests for insert_front and list_to_string.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ── Constants ───────────────────────────────────────────────────── */
#define FILENAME      "../2006Assignment3/Council_Member_Expenses_20260224.csv"
#define EXPECTED_COLS 10
#define COL_NAME      2
#define COL_AMOUNT    8
#define NAME_LEN      128
#define FIELD_LEN     512
#define RECORD_BUF    8192

/* ── Data types ──────────────────────────────────────────────────── */

/* One CSV row: at least 2 data fields (name + amount). */
typedef struct {
    char   name[NAME_LEN];
    double amount;
} Row;

/* Singly-linked list node. */
typedef struct Node {
    Row          data;
    struct Node *next;
} Node;

/* ── CSV helpers (reused from Assignment 3) ──────────────────────── */

static int read_csv_record(FILE *fp, char *buf, int buf_size)
{
    int pos = 0, in_quote = 0;
    while (1) {
        int c = fgetc(fp);
        if (c == EOF) {
            if (pos > 0) { buf[pos] = '\0'; return 1; }
            return 0;
        }
        if (c == '\r') continue;
        if (in_quote) {
            if (c == '"') {
                int next = fgetc(fp);
                if (next == '"') {
                    if (pos < buf_size - 2) { buf[pos++] = '"'; buf[pos++] = '"'; }
                } else {
                    if (next != EOF) ungetc(next, fp);
                    in_quote = 0;
                    if (pos < buf_size - 1) buf[pos++] = '"';
                }
            } else {
                if (pos < buf_size - 1) buf[pos++] = (char)c;
            }
        } else {
            if (c == '\n') {
                if (pos == 0) continue;
                buf[pos] = '\0';
                return 1;
            } else if (c == '"') {
                in_quote = 1;
                if (pos < buf_size - 1) buf[pos++] = '"';
            } else {
                if (pos < buf_size - 1) buf[pos++] = (char)c;
            }
        }
    }
}

static int parse_csv_line(const char *record, char fields[][FIELD_LEN], int max_fields)
{
    int n = 0;
    const char *p = record;
    while (n < max_fields) {
        char *out = fields[n];
        int   len = 0;
        if (*p == '"') {
            p++;
            while (*p) {
                if (*p == '"') {
                    if (*(p + 1) == '"') { if (len < FIELD_LEN - 1) out[len++] = '"'; p += 2; }
                    else { p++; break; }
                } else {
                    if (len < FIELD_LEN - 1) out[len++] = *p;
                    p++;
                }
            }
        } else {
            while (*p && *p != ',' && *p != '\n' && *p != '\r') {
                if (len < FIELD_LEN - 1) out[len++] = *p;
                p++;
            }
        }
        out[len] = '\0';
        n++;
        if (*p == ',') p++;
        else break;
    }
    return n;
}

static int parse_amount(const char *s, double *out)
{
    char buf[64]; int j = 0;
    for (int i = 0; s[i] && j < (int)(sizeof(buf) - 1); i++) {
        char c = s[i];
        if (c == '$' || c == ',' || c == ' ') continue;
        buf[j++] = c;
    }
    buf[j] = '\0';
    if (j == 0) return 0;
    char *end;
    double val = strtod(buf, &end);
    if (end == buf || *end != '\0') return 0;
    *out = val;
    return 1;
}

/* ── Linked-list functions ───────────────────────────────────────── */

/* Insert a new Row at the front of the list; returns new head. */
Node *insert_front(Node *head, const char *name, double amount)
{
    Node *n = (Node *)malloc(sizeof(Node));
    if (!n) { fprintf(stderr, "malloc failed\n"); exit(1); }
    strncpy(n->data.name, name, NAME_LEN - 1);
    n->data.name[NAME_LEN - 1] = '\0';
    n->data.amount = amount;
    n->next = head;
    return n;
}

/*
 * list_to_string
 * Returns a heap-allocated string "[name/$amt, name/$amt, ...]".
 * Caller must free() the result.
 */
char *list_to_string(const Node *head)
{
    /* Pass 1: measure required buffer length. */
    size_t needed = 3; /* "[]" + NUL */
    for (const Node *cur = head; cur; cur = cur->next)
        needed += strlen(cur->data.name) + 24; /* "$XX,XXX.XX, " */

    char *buf = (char *)malloc(needed);
    if (!buf) { fprintf(stderr, "malloc failed\n"); exit(1); }

    char *p = buf;
    *p++ = '[';
    for (const Node *cur = head; cur; cur = cur->next) {
        p += sprintf(p, "%s/$%.2f", cur->data.name, cur->data.amount);
        if (cur->next) { *p++ = ','; *p++ = ' '; }
    }
    *p++ = ']';
    *p   = '\0';
    return buf;
}

/* Free the entire list. */
static void free_list(Node *head)
{
    while (head) {
        Node *tmp = head->next;
        free(head);
        head = tmp;
    }
}

/* ── Memory reporting ────────────────────────────────────────────── */

static void print_memory_usage(void)
{
    FILE *fp = fopen("/proc/self/status", "r");
    if (!fp) { fprintf(stderr, "Cannot open /proc/self/status (not Linux?)\n"); return; }
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            printf("Memory usage (VmSize):%s", line + 7);
            break;
        }
    }
    fclose(fp);
}

/* ── Self-tests ──────────────────────────────────────────────────── */

static void run_tests(void)
{
    printf("=== Linked List Tests ===\n");

    /* Test 1: empty list toString */
    Node *head = NULL;
    char *s = list_to_string(head);
    printf("Empty list        : %s\n", s);  /* expect [] */
    free(s);

    /* Test 2: insert one item */
    head = insert_front(head, "Alice", 100.00);
    s = list_to_string(head);
    printf("After insert Alice: %s\n", s);  /* [Alice/$100.00] */
    free(s);

    /* Test 3: insert more items – each goes to the front */
    head = insert_front(head, "Bob",   50.25);
    head = insert_front(head, "Carol", 200.00);
    s = list_to_string(head);
    printf("After Bob, Carol  : %s\n", s);  /* [Carol/$200.00, Bob/$50.25, Alice/$100.00] */
    free(s);

    /* Test 4: verify order */
    printf("Head name         : %s  (expected Carol)\n", head->data.name);
    printf("Head->next name   : %s  (expected Bob)\n",   head->next->data.name);

    /* Test 5: insert and verify tail is still reachable */
    Node *cur = head;
    while (cur->next) cur = cur->next;
    printf("Tail name         : %s  (expected Alice)\n", cur->data.name);

    free_list(head);
    printf("=========================\n\n");
}

/* ── Main ────────────────────────────────────────────────────────── */

int main(void)
{
    run_tests();

    FILE *fp = fopen(FILENAME, "r");
    if (!fp) {
        fprintf(stderr, "Error: cannot open '%s'.\n", FILENAME);
        return 1;
    }

    char record[RECORD_BUF];
    char fields[EXPECTED_COLS][FIELD_LEN];

    /* Skip header row */
    if (!read_csv_record(fp, record, sizeof(record))) {
        fprintf(stderr, "Error: file is empty.\n");
        fclose(fp);
        return 1;
    }

    Node *head = NULL;

    while (read_csv_record(fp, record, sizeof(record))) {
        int n = parse_csv_line(record, fields, EXPECTED_COLS);
        if (n != EXPECTED_COLS) continue;

        const char *name = fields[COL_NAME];
        double amount;
        if (strlen(name) == 0) continue;
        if (!parse_amount(fields[COL_AMOUNT], &amount)) continue;

        head = insert_front(head, name, amount);
    }
    fclose(fp);

    printf("CSV loaded into linked list.\n");
    print_memory_usage();

    /* Print first 5 nodes as a sample */
    printf("\nFirst 5 nodes:\n");
    int cnt = 0;
    for (Node *cur = head; cur && cnt < 5; cur = cur->next, cnt++)
        printf("  %s  $%.2f\n", cur->data.name, cur->data.amount);

    free_list(head);
    return 0;
}
