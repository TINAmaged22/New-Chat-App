#include <gtk/gtk.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <cstring>
#include <vector>
#include <string>

#define SHM_SIZE 1024
#define BASE_SHM_KEY 2000

// Structure to hold chat tab data
struct ChatTab {
    std::string contact_name;
    int shm_key;
    char* shm_ptr;
    int shmid;
    GtkWidget* text_view;
    GtkTextBuffer* buffer;
    GtkWidget* entry;
    char last_message[SHM_SIZE];
};

std::vector<ChatTab*> chat_tabs;
std::string my_username;
GtkWidget* notebook;

// Timer function to check for new messages in all tabs
gboolean check_messages(gpointer data) {
    for (ChatTab* tab : chat_tabs) {
        if (tab->shm_ptr != nullptr) {
            char current_message[SHM_SIZE];
            strncpy(current_message, tab->shm_ptr, SHM_SIZE - 1);
            current_message[SHM_SIZE - 1] = '\0';

            // Only display if message has changed and is not empty
            if (strlen(current_message) > 0 && strcmp(current_message, tab->last_message) != 0) {
                // Update the text view
                GtkTextIter end;
                gtk_text_buffer_get_end_iter(tab->buffer, &end);

                char display_text[SHM_SIZE + 20];
                snprintf(display_text, sizeof(display_text), "%s\n", current_message);
                gtk_text_buffer_insert(tab->buffer, &end, display_text, -1);

                // Scroll to bottom
                gtk_text_buffer_get_end_iter(tab->buffer, &end);
                GtkTextMark *mark = gtk_text_buffer_create_mark(tab->buffer, NULL, &end, FALSE);
                gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(tab->text_view), mark, 0.0, TRUE, 0.0, 1.0);
                gtk_text_buffer_delete_mark(tab->buffer, mark);

                // Save this message
                strncpy(tab->last_message, current_message, SHM_SIZE - 1);
            }
        }
    }
    return TRUE; // Keep the timer running
}

// Function called when Send button is clicked
void on_send_clicked(GtkWidget *button, gpointer data) {
    ChatTab* tab = (ChatTab*)data;
    const char* message = gtk_entry_get_text(GTK_ENTRY(tab->entry));

    if (strlen(message) > 0) {
        // Format: "MyName: message"
        char full_message[SHM_SIZE];
        snprintf(full_message, SHM_SIZE, "%s: %s", my_username.c_str(), message);

        // Write message to shared memory
        strncpy(tab->shm_ptr, full_message, SHM_SIZE - 1);
        tab->shm_ptr[SHM_SIZE - 1] = '\0';

        // Also display in our own text view
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(tab->buffer, &end);
        char display_text[SHM_SIZE + 20];
        snprintf(display_text, sizeof(display_text), "%s\n", full_message);
        gtk_text_buffer_insert(tab->buffer, &end, display_text, -1);

        // Scroll to bottom
        gtk_text_buffer_get_end_iter(tab->buffer, &end);
        GtkTextMark *mark = gtk_text_buffer_create_mark(tab->buffer, NULL, &end, FALSE);
        gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(tab->text_view), mark, 0.0, TRUE, 0.0, 1.0);
        gtk_text_buffer_delete_mark(tab->buffer, mark);

        // Clear the input field
        gtk_entry_set_text(GTK_ENTRY(tab->entry), "");
    }
}

// Function called when Enter key is pressed
void on_entry_activate(GtkEntry *entry, gpointer data) {
    ChatTab* tab = (ChatTab*)data;
    on_send_clicked(NULL, tab);
}

// Create a new chat tab
void create_chat_tab(const char* contact_name, int shm_key) {
    ChatTab* tab = new ChatTab();
    tab->contact_name = contact_name;
    tab->shm_key = shm_key;
    memset(tab->last_message, 0, SHM_SIZE);

    // Create or access shared memory for this contact
    tab->shmid = shmget(shm_key, SHM_SIZE, 0666 | IPC_CREAT);
    if (tab->shmid == -1) {
        g_print("Failed to create shared memory for %s\n", contact_name);
        delete tab;
        return;
    }

    // Attach to shared memory
    tab->shm_ptr = (char*)shmat(tab->shmid, NULL, 0);
    if (tab->shm_ptr == (char*)-1) {
        g_print("Failed to attach shared memory for %s\n", contact_name);
        delete tab;
        return;
    }

    // Create the tab content container
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    // Create scrolled window for messages
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    // Create text view for messages
    tab->text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tab->text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tab->text_view), GTK_WRAP_WORD);
    tab->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tab->text_view));
    gtk_container_add(GTK_CONTAINER(scrolled_window), tab->text_view);

    // Create horizontal box for input and button
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    // Create text entry
    tab->entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(tab->entry), "Type a message...");
    g_signal_connect(tab->entry, "activate", G_CALLBACK(on_entry_activate), tab);
    gtk_box_pack_start(GTK_BOX(hbox), tab->entry, TRUE, TRUE, 0);

    // Create Send button
    GtkWidget *button = gtk_button_new_with_label("Send");
    g_signal_connect(button, "clicked", G_CALLBACK(on_send_clicked), tab);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    // Add tab to notebook
    GtkWidget *tab_label = gtk_label_new(contact_name);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, tab_label);
    gtk_widget_show_all(vbox);

    // Add to our tab list
    chat_tabs.push_back(tab);
}

// Dialog to add new chat
void on_new_chat_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("New Chat",
                                                     GTK_WINDOW(data),
                                                     GTK_DIALOG_MODAL,
                                                     "Cancel", GTK_RESPONSE_CANCEL,
                                                     "Create", GTK_RESPONSE_OK,
                                                     NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *label1 = gtk_label_new("Contact Name:");
    gtk_box_pack_start(GTK_BOX(vbox), label1, FALSE, FALSE, 0);

    GtkWidget *name_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(name_entry), "Enter contact name");
    gtk_box_pack_start(GTK_BOX(vbox), name_entry, FALSE, FALSE, 0);

    GtkWidget *label2 = gtk_label_new("Room Number (1-999):");
    gtk_box_pack_start(GTK_BOX(vbox), label2, FALSE, FALSE, 0);

    GtkWidget *room_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(room_entry), "Enter room number");
    gtk_box_pack_start(GTK_BOX(vbox), room_entry, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_OK) {
        const char* contact_name = gtk_entry_get_text(GTK_ENTRY(name_entry));
        const char* room_str = gtk_entry_get_text(GTK_ENTRY(room_entry));

        if (strlen(contact_name) > 0 && strlen(room_str) > 0) {
            int room_number = atoi(room_str);
            if (room_number > 0 && room_number < 1000) {
                int shm_key = BASE_SHM_KEY + room_number;
                create_chat_tab(contact_name, shm_key);
            }
        }
    }

    gtk_widget_destroy(dialog);
}

// Cleanup when window is closed
void on_window_destroy(GtkWidget *widget, gpointer data) {
    // Detach all shared memory
    for (ChatTab* tab : chat_tabs) {
        if (tab->shm_ptr != nullptr) {
            shmdt(tab->shm_ptr);
        }
        delete tab;
    }
    chat_tabs.clear();
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    // Get username from command line
    if (argc > 1) {
        my_username = argv[1];
    } else {
        char username_input[256];
        g_print("Enter your name: ");
        if (fgets(username_input, sizeof(username_input), stdin) != NULL) {
            username_input[strcspn(username_input, "\n")] = 0;
            my_username = username_input;
        }
        if (my_username.empty()) {
            my_username = "Anonymous";
        }
    }

    // Initialize GTK
    gtk_init(&argc, &argv);

    // Create main window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    char window_title[300];
    snprintf(window_title, sizeof(window_title), "Chat - %s", my_username.c_str());
    gtk_window_set_title(GTK_WINDOW(window), window_title);
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    gtk_container_set_border_width(GTK_CONTAINER(window), 0);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    // Create main vertical box
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), main_vbox);

    // Create toolbar
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(toolbar), 5);
    gtk_box_pack_start(GTK_BOX(main_vbox), toolbar, FALSE, FALSE, 0);

    // Add "New Chat" button
    GtkWidget *new_chat_btn = gtk_button_new_with_label("+ New Chat");
    g_signal_connect(new_chat_btn, "clicked", G_CALLBACK(on_new_chat_clicked), window);
    gtk_box_pack_start(GTK_BOX(toolbar), new_chat_btn, FALSE, FALSE, 0);

    // Create notebook (tabs)
    notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
    gtk_box_pack_start(GTK_BOX(main_vbox), notebook, TRUE, TRUE, 0);

    // Add timer to check for messages every 100ms
    g_timeout_add(100, check_messages, NULL);

    // Show all widgets
    gtk_widget_show_all(window);

    // Run GTK main loop
    gtk_main();

    return 0;
}
