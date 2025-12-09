#include <gtk/gtk.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <cstring>

#define SHM_SIZE 1024
#define SHM_KEY 1234

char* shm_ptr = nullptr;
int shmid;
char username[256] = {0};

// Function called when Send button is clicked
void on_send_clicked(GtkWidget *button, gpointer data) {
    GtkEntry *entry = GTK_ENTRY(data);
    const char* message = gtk_entry_get_text(entry);

    if (strlen(message) > 0) {
        // Format: "username: message"
        char full_message[SHM_SIZE];
        snprintf(full_message, SHM_SIZE, "%s: %s", username, message);

        // Write message to shared memory
        strncpy(shm_ptr, full_message, SHM_SIZE - 1);
        shm_ptr[SHM_SIZE - 1] = '\0';

        // Clear the input field
        gtk_entry_set_text(entry, "");
    }
}

// Function called when Enter key is pressed
void on_entry_activate(GtkEntry *entry, gpointer data) {
    on_send_clicked(NULL, entry);
}

// Cleanup when window is closed
void on_window_destroy(GtkWidget *widget, gpointer data) {
    if (shm_ptr != nullptr) {
        shmdt(shm_ptr);
    }
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    // Get username from command line or prompt
    if (argc > 1) {
        strncpy(username, argv[1], 255);
    } else {
        g_print("Enter your name: ");
        if (fgets(username, sizeof(username), stdin) != NULL) {
            // Remove newline
            username[strcspn(username, "\n")] = 0;
        }
        if (strlen(username) == 0) {
            strcpy(username, "Anonymous");
        }
    }

    // Create shared memory
    shmid = shmget(SHM_KEY, SHM_SIZE, 0666 | IPC_CREAT);
    if (shmid == -1) {
        g_print("Failed to create shared memory\n");
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
    char window_title[300];
    snprintf(window_title, sizeof(window_title), "Chat Writer - %s", username);
    gtk_window_set_title(GTK_WINDOW(window), window_title);
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 100);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    // Create vertical box container
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Create label
    GtkWidget *label = gtk_label_new("Type your message:");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    // Create horizontal box for input and button
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    // Create text entry
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter message here...");
    g_signal_connect(entry, "activate", G_CALLBACK(on_entry_activate), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);

    // Create Send button
    GtkWidget *button = gtk_button_new_with_label("Send");
    g_signal_connect(button, "clicked", G_CALLBACK(on_send_clicked), entry);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    // Show all widgets
    gtk_widget_show_all(window);

    // Run GTK main loop
    gtk_main();

    return 0;
}
