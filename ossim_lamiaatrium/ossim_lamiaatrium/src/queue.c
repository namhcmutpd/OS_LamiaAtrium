#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t *q)
{
        if (q == NULL)
                return 1;
        return (q->size == 0);
}

void enqueue(struct queue_t *q, struct pcb_t *proc)
{
        /* TODO: put a new process to queue [q] */
        if (q == NULL || proc == NULL)
                return;
        if (q->size < MAX_QUEUE_SIZE) {
                q->proc[q->size] = proc;
                q->size++;
        }
}

struct pcb_t *dequeue(struct queue_t *q)
{
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if (q == NULL || q->size == 0)
                return NULL;

        /* Lấy process đầu tiên trong queue (FIFO cho MLQ) */
        struct pcb_t *proc = q->proc[0];

        /* Dồn mảng để xóa phần tử đầu */
        for (int i = 0; i < q->size - 1; i++) {
                q->proc[i] = q->proc[i + 1];
        }
        q->size--;

        return proc;
}

struct pcb_t *purgequeue(struct queue_t *q, struct pcb_t *proc)
{
        /* TODO: remove a specific item from queue
         * */
        if (q == NULL || proc == NULL || q->size == 0)
                return NULL;

        /* Tìm process trong queue */
        int found_idx = -1;
        for (int i = 0; i < q->size; i++) {
                if (q->proc[i] == proc) {
                        found_idx = i;
                        break;
                }
        }

        if (found_idx == -1)
                return NULL;

        struct pcb_t *removed = q->proc[found_idx];

        /* Dồn mảng để xóa phần tử */
        for (int i = found_idx; i < q->size - 1; i++) {
                q->proc[i] = q->proc[i + 1];
        }
        q->size--;

        return removed;
}