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

// Structure for individual messages
struct Message {
    std::string sender;
    std::string text;
    bool is_mine;
};

// Structure to hold chat tab data
struct ChatTab {
    std::string contact_name;
    int room_number;
    std::vector<Message> messages;
    char last_message[SHM_SIZE];
    float scroll_offset;

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
    tab->messages.clear();
    memset(tab->last_message, 0, SHM_SIZE);
    tab->scroll_offset = 0;

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
                // Parse message "sender: text"
                std::string msg_str(current_message);
                size_t colon_pos = msg_str.find(": ");

                if (colon_pos != std::string::npos) {
                    Message msg;
                    msg.sender = msg_str.substr(0, colon_pos);
                    msg.text = msg_str.substr(colon_pos + 2);
                    msg.is_mine = (msg.sender == my_username);

                    tab->messages.push_back(msg);
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

    // Add to our own message list
    Message msg;
    msg.sender = my_username;
    msg.text = message;
    msg.is_mine = true;
    tab->messages.push_back(msg);
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
        GuiPanel((Rectangle){ 0, 0, screenWidth, 50 }, NULL);

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

            // Chat history area header
            GuiLabel((Rectangle){ 20, 110, 200, 20 }, TextFormat("Chat with %s (Room %d)",
                     current_tab->contact_name.c_str(), current_tab->room_number));

            // Chat area background
            Rectangle chat_area = { 20, 140, screenWidth - 40, 280 };
            DrawRectangle(chat_area.x, chat_area.y, chat_area.width, chat_area.height, (Color){240, 240, 240, 255});
            DrawRectangleLines(chat_area.x, chat_area.y, chat_area.width, chat_area.height, DARKGRAY);

            // Handle scrolling with mouse wheel
            float mouse_wheel = GetMouseWheelMove();
            if (mouse_wheel != 0) {
                current_tab->scroll_offset -= mouse_wheel * 20;
                if (current_tab->scroll_offset < 0) current_tab->scroll_offset = 0;
            }

            // Clip messages to chat area
            BeginScissorMode(chat_area.x, chat_area.y, chat_area.width, chat_area.height);

            // Draw messages
            int y_pos = chat_area.y + 10 - (int)current_tab->scroll_offset;
            int line_height = 20;

            for (size_t i = 0; i < current_tab->messages.size(); i++) {
                const Message& msg = current_tab->messages[i];

                // Calculate message box dimensions
                int msg_width = MeasureText(msg.text.c_str(), 10) + 20;
                if (msg_width > chat_area.width - 60) msg_width = chat_area.width - 60;
                int msg_height = line_height + 10;

                int msg_x;
                Color box_color;

                if (msg.is_mine) {
                    // My messages on the right (green)
                    msg_x = chat_area.x + chat_area.width - msg_width - 10;
                    box_color = (Color){200, 255, 200, 255};
                } else {
                    // Their messages on the left (white)
                    msg_x = chat_area.x + 10;
                    box_color = WHITE;
                }

                // Draw message box
                DrawRectangle(msg_x, y_pos, msg_width, msg_height, box_color);
                DrawRectangleLines(msg_x, y_pos, msg_width, msg_height, GRAY);

                // Draw message text
                if (!msg.is_mine) {
                    // Show sender name for others
                    DrawText(msg.sender.c_str(), msg_x + 5, y_pos + 2, 8, DARKGRAY);
                    DrawText(msg.text.c_str(), msg_x + 10, y_pos + 12, 10, BLACK);
                    msg_height += 10;
                } else {
                    DrawText(msg.text.c_str(), msg_x + 10, y_pos + 5, 10, BLACK);
                }

                y_pos += msg_height + 5;
            }

            EndScissorMode();

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
            GuiPanel(dialog, "New Chat");

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
