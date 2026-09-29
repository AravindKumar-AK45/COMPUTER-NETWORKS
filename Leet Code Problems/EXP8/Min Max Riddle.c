#include <stdio.h>
#include <stdlib.h>

int main() {
    int n;
    scanf("%d", &n);

    // Dynamic allocation - no size limits
    long long *arr  = (long long *)malloc(n * sizeof(long long));
    int *left       = (int *)malloc(n * sizeof(int));
    int *right      = (int *)malloc(n * sizeof(int));
    long long *ans  = (long long *)calloc(n + 2, sizeof(long long));
    int *stk        = (int *)malloc(n * sizeof(int));

    // Read input
    for (int i = 0; i < n; i++)
        scanf("%lld", &arr[i]);

    // ---- LEFT SPAN (monotonic increasing stack) ----
    // Find how far left arr[i] can be the minimum
    int top = -1;
    for (int i = 0; i < n; i++) {
        while (top >= 0 && arr[stk[top]] >= arr[i])
            top--;
        left[i] = (top == -1) ? (i + 1) : (i - stk[top]);
        stk[++top] = i;
    }

    // ---- RIGHT SPAN (monotonic decreasing stack) ----
    // Find how far right arr[i] can be the minimum
    top = -1;
    for (int i = n - 1; i >= 0; i--) {
        while (top >= 0 && arr[stk[top]] > arr[i])
            top--;
        right[i] = (top == -1) ? (n - i) : (stk[top] - i);
        stk[++top] = i;
    }

    // For each element: max window where it's the minimum
    for (int i = 0; i < n; i++) {
        int mw = left[i] + right[i] - 1;
        if (mw <= n && arr[i] > ans[mw])
            ans[mw] = arr[i];
    }

    // PROPAGATE: smaller windows inherit from larger windows
    for (int i = n - 1; i >= 1; i--) {
        if (ans[i] < ans[i + 1])
            ans[i] = ans[i + 1];
    }

    // Output
    for (int i = 1; i <= n; i++)
        printf("%lld ", ans[i]);
    printf("\n");

    free(arr);
    free(left);
    free(right);
    free(ans);
    free(stk);

    return 0;
}
