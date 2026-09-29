#include <stdio.h>
#include <stdbool.h>

int main() {
    int window_size, total_frames, lost_frame;

    // Get user input
    printf("Enter window size (N): ");
    scanf("%d", &window_size);
    printf("Enter total number of frames (F): ");
    scanf("%d", &total_frames);
    printf("Enter frame to lose (0 to %d): ", total_frames - 1);
    scanf("%d", &lost_frame);

    int send_base = 0;
    int send_top = (window_size < total_frames) ? window_size - 1 : total_frames - 1;
    int total_transmissions = 0;
    int retransmissions = 0;
    bool ack_received[total_frames];
    bool frame_sent[total_frames];
    bool frame_lost[total_frames] = {false};
    frame_lost[lost_frame] = true;

    // Initialize ACK and frame status
    for (int i = 0; i < total_frames; i++) {
        ack_received[i] = false;
        frame_sent[i] = false;
    }

    printf("\n=== Selective Repeat Simulation Steps ===\n");
    while (send_base < total_frames) {
        // Sender sends frames in the window
        for (int i = send_base; i <= send_top; i++) {
            if (!frame_sent[i]) {
                frame_sent[i] = true;
                total_transmissions++;
                printf("Sender: Sending Frame %d\n", i);
                if (frame_lost[i]) {
                    printf("Receiver: Frame %d is LOST\n", i);
                } else {
                    ack_received[i] = true;
                    printf("Receiver: ACK for Frame %d received\n", i);
                }
            }
        }

        // Update send_base if ACK is received
        while (send_base < total_frames && ack_received[send_base]) {
            send_base++;
            if (send_base + window_size - 1 < total_frames) {
                send_top = send_base + window_size - 1;
            } else {
                send_top = total_frames - 1;
            }
        }

        // Retransmit ONLY the lost frame (if ACK for send_base is missing)
        if (send_base < total_frames && !ack_received[send_base]) {
            printf("Sender: Timeout! Retransmitting Frame %d (Selective)\n", send_base);
            retransmissions++;
            frame_sent[send_base] = false; // Reset only this frame
        }
    }

    // Final Statistics (with Efficiency)
    printf("\n=== Statistics ===\n");
    printf("Total Transmissions: %d\n", total_transmissions);
    printf("Total Retransmissions: %d\n", retransmissions);
    printf("Successful Frame Delivery: %d\n", total_frames);
    printf("Efficiency: %.2f%%\n", (float)total_frames / total_transmissions * 100);
    return 0;
}