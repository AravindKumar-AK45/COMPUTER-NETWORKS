#include <stdlib.h>

/**
 * Note: The returned array must be malloced, assume caller calls free().
 */
int* maxSlidingWindow(int* nums, int numsSize, int k, int* returnSize) {
    *returnSize = 0;
    if (k <= 0 || k > numsSize) return NULL;

    int outputSize = numsSize - k + 1;
    int* result = malloc((size_t)outputSize * sizeof *result);
    int* deque = malloc((size_t)k * sizeof *deque);

    if (!result || !deque) {
        free(result);
        free(deque);
        return NULL;
    }

    int head = 0, tail = 0, count = 0;

    for (int i = 0; i < numsSize; i++) {
        /* Remove indices outside the window. */
        while (count && deque[head] <= i - k) {
            head = (head + 1) % k;
            count--;
        }

        /* Remove values that cannot be a future maximum. */
        while (count) {
            int last = (tail + k - 1) % k;
            if (nums[deque[last]] > nums[i]) break;
            tail = last;
            count--;
        }

        deque[tail] = i;
        tail = (tail + 1) % k;
        count++;

        if (i >= k - 1)
            result[i - k + 1] = nums[deque[head]];
    }

    free(deque);
    *returnSize = outputSize;
    return result;
}
