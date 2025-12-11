#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <vector>
#include <string>
#include <cstring>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/ipc.h>
    #include <sys/shm.h>
#endif

#define SHM_SIZE 1024
#define BASE_SHM_KEY 2000
#define MAX_TABS 10
#define MAX_MESSAGE_LENGTH 256

// Structure to hold chat tab data
struct ChatTab {
    std::string contact_name;
    int room_number;
    char chat_history[4096];
    char last_message[SHM_SIZE];

#ifdef _WIN32
    HANDLE hMapFile;
    char* shm_ptr;
#else
    int shmid;
    char* shm_ptr;
#endif
};

std::vector<ChatTab*> chat_tabs;
int active_tab = 0;
std::string my_username;

// Dialog state
bool show_new_chat_dialog = false;
char contact_name_input[64] = "";
char room_number_input[16] = "";
bool contact_edit_mode = false;
bool room_edit_mode = false;

// Create or access shared memory
char* create_shared_memory(int room_number) {
#ifdef _WIN32
    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "ChatRoom%d", room_number);

    HANDLE hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        SHM_SIZE,
        shm_name);

    if (hMapFile == NULL) {
        return nullptr;
    }

    char* ptr = (char*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE);
    return ptr;
#else
    int shm_key = BASE_SHM_KEY + room_number;
    int shmid = shmget(shm_key, SHM_SIZE, 0666 | IPC_CREAT);
    if (shmid == -1) {
        return nullptr;
    }

    char* ptr = (char*)shmat(shmid, NULL, 0);
    if (ptr == (char*)-1) {
        return nullptr;
    }

    return ptr;
#endif
}

// Create a new chat tab
void create_chat_tab(const char* contact_name, int room_number) {
    if (chat_tabs.size() >= MAX_TABS) {
        return; // Max tabs reached
    }

    ChatTab* tab = new ChatTab();
    tab->contact_name = contact_name;
    tab->room_number = room_number;
    memset(tab->chat_history, 0, sizeof(tab->chat_history));
    memset(tab->last_message, 0, SHM_SIZE);

    // Create shared memory
    tab->shm_ptr = create_shared_memory(room_number);

    if (tab->shm_ptr == nullptr) {
        delete tab;
        return;
    }

    chat_tabs.push_back(tab);
    active_tab = chat_tabs.size() - 1; // Switch to new tab
}

// Check for new messages in all tabs
void check_messages() {
    for (ChatTab* tab : chat_tabs) {
        if (tab->shm_ptr != nullptr) {
            char current_message[SHM_SIZE];
            strncpy(current_message, tab->shm_ptr, SHM_SIZE - 1);
            current_message[SHM_SIZE - 1] = '\0';

            // Only display if message has changed and is not empty
            if (strlen(current_message) > 0 && strcmp(current_message, tab->last_message) != 0) {
                // Add to chat history
                if (strlen(tab->chat_history) + strlen(current_message) + 2 < sizeof(tab->chat_history)) {
                    strcat(tab->chat_history, current_message);
                    strcat(tab->chat_history, "\n");
                }

                // Save this message
                strncpy(tab->last_message, current_message, SHM_SIZE - 1);
            }
        }
    }
}

// Send message
void send_message(ChatTab* tab, const char* message) {
    if (tab == nullptr || tab->shm_ptr == nullptr) return;
    if (strlen(message) == 0) return;

    // Format: "MyName: message"
    char full_message[SHM_SIZE];
    snprintf(full_message, SHM_SIZE, "%s: %s", my_username.c_str(), message);

    // Write to shared memory
    strncpy(tab->shm_ptr, full_message, SHM_SIZE - 1);
    tab->shm_ptr[SHM_SIZE - 1] = '\0';

    // Update last_message to prevent duplication
    strncpy(tab->last_message, full_message, SHM_SIZE - 1);

    // Add to our own chat history
    if (strlen(tab->chat_history) + strlen(full_message) + 2 < sizeof(tab->chat_history)) {
        strcat(tab->chat_history, full_message);
        strcat(tab->chat_history, "\n");
    }
}

int main(int argc, char* argv[]) {
    // Get username
    if (argc > 1) {
        my_username = argv[1];
    } else {
        my_username = "User";
    }

    const int screenWidth = 700;
    const int screenHeight = 500;

    InitWindow(screenWidth, screenHeight, TextFormat("Chat - %s", my_username.c_str()));
    SetTargetFPS(60);

    char message_input[MAX_MESSAGE_LENGTH] = "";
    bool message_edit_mode = false;

    while (!WindowShouldClose()) {
        // Check for new messages
        check_messages();

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Top toolbar
        GuiPanel((Rectangle){ 0, 0, screenWidth, 50 });

        if (GuiButton((Rectangle){ 10, 10, 120, 30 }, "+ New Chat")) {
            show_new_chat_dialog = true;
            memset(contact_name_input, 0, sizeof(contact_name_input));
            memset(room_number_input, 0, sizeof(room_number_input));
        }

        GuiLabel((Rectangle){ 150, 10, 300, 30 }, TextFormat("Logged in as: %s", my_username.c_str()));

        // Draw tabs
        int tab_y = 60;
        int tab_height = 35;
        int tab_width = 150;

        for (size_t i = 0; i < chat_tabs.size(); i++) {
            Rectangle tab_rect = { 10 + (float)(i * (tab_width + 5)), (float)tab_y, (float)tab_width, (float)tab_height };

            if (i == active_tab) {
                GuiSetState(STATE_PRESSED);
            } else {
                GuiSetState(STATE_NORMAL);
            }

            if (GuiButton(tab_rect, chat_tabs[i]->contact_name.c_str())) {
                active_tab = i;
            }

            GuiSetState(STATE_NORMAL);
        }

        // Draw active tab content
        if (!chat_tabs.empty() && active_tab < chat_tabs.size()) {
            ChatTab* current_tab = chat_tabs[active_tab];

            // Chat history area
            GuiLabel((Rectangle){ 20, 110, 200, 20 }, TextFormat("Chat with %s (Room %d)",
                     current_tab->contact_name.c_str(), current_tab->room_number));

            GuiTextBoxMulti((Rectangle){ 20, 140, screenWidth - 40, 280 },
                           current_tab->chat_history, 4096, false);

            // Message input area
            GuiLabel((Rectangle){ 20, 430, 200, 20 }, "Type your message:");

            if (GuiTextBox((Rectangle){ 20, 455, screenWidth - 150, 35 },
                          message_input, MAX_MESSAGE_LENGTH, message_edit_mode)) {
                message_edit_mode = !message_edit_mode;
            }

            // Send button
            if (GuiButton((Rectangle){ screenWidth - 120, 455, 100, 35 }, "Send") ||
                (message_edit_mode && IsKeyPressed(KEY_ENTER))) {
                send_message(current_tab, message_input);
                memset(message_input, 0, sizeof(message_input));
                message_edit_mode = false;
            }
        } else {
            GuiLabel((Rectangle){ 20, 200, 400, 30 }, "No chats yet. Click '+ New Chat' to start!");
        }

        // New Chat Dialog
        if (show_new_chat_dialog) {
            // Semi-transparent background
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.5f));

            // Dialog box
            Rectangle dialog = { screenWidth/2 - 175, screenHeight/2 - 125, 350, 250 };
            GuiPanel(dialog);

            GuiLabel((Rectangle){ dialog.x + 20, dialog.y + 10, 200, 30 }, "New Chat");

            GuiLabel((Rectangle){ dialog.x + 20, dialog.y + 50, 150, 20 }, "Contact Name:");
            if (GuiTextBox((Rectangle){ dialog.x + 20, dialog.y + 75, 310, 30 },
                          contact_name_input, 64, contact_edit_mode)) {
                contact_edit_mode = !contact_edit_mode;
            }

            GuiLabel((Rectangle){ dialog.x + 20, dialog.y + 115, 150, 20 }, "Room Number (1-999):");
            if (GuiTextBox((Rectangle){ dialog.x + 20, dialog.y + 140, 310, 30 },
                          room_number_input, 16, room_edit_mode)) {
                room_edit_mode = !room_edit_mode;
            }

            // Buttons
            if (GuiButton((Rectangle){ dialog.x + 20, dialog.y + 190, 150, 40 }, "Cancel")) {
                show_new_chat_dialog = false;
            }

            if (GuiButton((Rectangle){ dialog.x + 180, dialog.y + 190, 150, 40 }, "Create")) {
                if (strlen(contact_name_input) > 0 && strlen(room_number_input) > 0) {
                    int room_num = atoi(room_number_input);
                    if (room_num > 0 && room_num < 1000) {
                        create_chat_tab(contact_name_input, room_num);
                        show_new_chat_dialog = false;
                    }
                }
            }
        }

        EndDrawing();
    }

    // Cleanup
#ifdef _WIN32
    for (ChatTab* tab : chat_tabs) {
        if (tab->shm_ptr != nullptr) {
            UnmapViewOfFile(tab->shm_ptr);
        }
        delete tab;
    }
#else
    for (ChatTab* tab : chat_tabs) {
        if (tab->shm_ptr != nullptr) {
            shmdt(tab->shm_ptr);
        }
        delete tab;
    }
#endif

    CloseWindow();
    return 0;
}
