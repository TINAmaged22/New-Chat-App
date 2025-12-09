#include <gtk/gtk.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <cstring>

#define SHM_SIZE 1024
#define SHM_KEY 1234

char* shm_ptr = nullptr;
int shmid;
GtkWidget *text_view;
GtkTextBuffer *buffer;
char last_message[SHM_SIZE] = {0};

// Timer function to check for new messages
gboolean check_messages(gpointer data) {
    if (shm_ptr != nullptr) {
        char current_message[SHM_SIZE];
        strncpy(current_message, shm_ptr, SHM_SIZE - 1);
        current_message[SHM_SIZE - 1] = '\0';

        // Only display if message has changed and is not empty
        if (strlen(current_message) > 0 && strcmp(current_message, last_message) != 0) {
            // Update the text view
            GtkTextIter end;
            gtk_text_buffer_get_end_iter(buffer, &end);

            char display_text[SHM_SIZE + 20];
            snprintf(display_text, sizeof(display_text), "%s\n", current_message);
            gtk_text_buffer_insert(buffer, &end, display_text, -1);

            // Scroll to bottom
            gtk_text_buffer_get_end_iter(buffer, &end);
            GtkTextMark *mark = gtk_text_buffer_create_mark(buffer, NULL, &end, FALSE);
            gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(text_view), mark, 0.0, TRUE, 0.0, 1.0);
            gtk_text_buffer_delete_mark(buffer, mark);

            // Save this message
            strncpy(last_message, current_message, SHM_SIZE - 1);
        }
    }
    return TRUE; // Keep the timer running
}

// Cleanup when window is closed
void on_window_destroy(GtkWidget *widget, gpointer data) {
    if (shm_ptr != nullptr) {
        shmdt(shm_ptr);
    }
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    // Access shared memory
    shmid = shmget(SHM_KEY, SHM_SIZE, 0666 | IPC_CREAT);
    if (shmid == -1) {
        g_print("Failed to access shared memory\n");
        return 1;
    }

    // Attach to shared memory
    shm_ptr = (char*)shmat(shmid, NULL, 0);
    if (shm_ptr == (char*)-1) {
        g_print("Failed to attach shared memory\n");
        return 1;
    }

    // Initialize GTK
    gtk_init(&argc, &argv);

    // Create main window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Chat Reader");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    // Create vertical box container
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Create label
    GtkWidget *label = gtk_label_new("Messages:");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    // Create scrolled window
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    // Create text view
    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

    // Add timer to check for messages every 100ms
    g_timeout_add(100, check_messages, NULL);

    // Show all widgets
    gtk_widget_show_all(window);

    // Run GTK main loop
    gtk_main();

    return 0;
}
