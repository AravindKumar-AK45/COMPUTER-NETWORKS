#include <stdlib.h>
#include <string.h>

static int compareInts(const void *a, const void *b) {
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y);  // Avoid overflow from x - y.
}

/* Return the 1-based position of value in sorted[]. */
static int compressedIndex(const int *sorted, int size, int value) {
    int left = 0, right = size;

    while (left < right) {
        int mid = left + (right - left) / 2;
        if (sorted[mid] < value)
            left = mid + 1;
        else
            right = mid;
    }
    return left + 1;
}

static void add(int *tree, int size, int index, int delta) {
    while (index <= size) {
        tree[index] += delta;
        index += index & -index;
    }
}

/* Return the 0-based sorted[] index of the kth smallest value. */
static int kth(const int *tree, int size, int k) {
    int pos = 0;
    int step = 1;
    while (step <= size / 2) step *= 2;

    while (step > 0) {
        int next = pos + step;
        if (next <= size && tree[next] < k) {
            pos = next;
            k -= tree[next];
        }
        step /= 2;
    }
    return pos;
}

/**
 * Note: The returned array must be malloced, assume caller calls free().
 */
double* medianSlidingWindow(int* nums, int numsSize, int k, int* returnSize) {
    *returnSize = 0;
    if (k <= 0 || k > numsSize) return NULL;

    int count = numsSize - k + 1;
    int *sorted = malloc((size_t)numsSize * sizeof(*sorted));
    double *result = malloc((size_t)count * sizeof(*result));

    if (!sorted || !result) {
        free(sorted);
        free(result);
        return NULL;
    }

    memcpy(sorted, nums, (size_t)numsSize * sizeof(*sorted));
    qsort(sorted, numsSize, sizeof(*sorted), compareInts);

    int unique = 0;
    for (int i = 0; i < numsSize; i++) {
        if (unique == 0 || sorted[i] != sorted[unique - 1])
            sorted[unique++] = sorted[i];
    }

    int *tree = calloc((size_t)unique + 1, sizeof(*tree));
    if (!tree) {
        free(sorted);
        free(result);
        return NULL;
    }

    for (int i = 0; i < numsSize; i++) {
        add(tree, unique, compressedIndex(sorted, unique, nums[i]), 1);

        if (i >= k)
            add(tree, unique, compressedIndex(sorted, unique, nums[i - k]), -1);

        if (i >= k - 1) {
            int left = sorted[kth(tree, unique, (k + 1) / 2)];

            if (k % 2) {
                result[i - k + 1] = (double)left;
            } else {
                int right = sorted[kth(tree, unique, k / 2 + 1)];
                result[i - k + 1] = ((double)left + (double)right) / 2.0;
            }
        }
    }

    free(tree);
    free(sorted);
    *returnSize = count;
    return result;
}
