#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <iostream>
#include <cstring>
#include <vector>
#include <string>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define SHM_SIZE 1024

// Message structure
struct Message {
    std::string sender;
    std::string text;
    bool is_mine;
};

// Global variables
std::vector<Message> chat_messages;
std::string my_username;
std::string last_message = "";
float scroll_offset = 0;

// Shared memory functions (from your shared_memo)
key_t get_key() {
    key_t shm_key = ftok("shmfile", 65);
    return shm_key;
}

int share_memory(key_t shm_key) {
    int shm_id = shmget(shm_key, SHM_SIZE, 0666 | IPC_CREAT);
    if (shm_id == -1) {
        std::cerr << "Failed to access shared memory" << std::endl;
        return -1;
    }
    return shm_id;
}

char* shm_access(int shm_id) {
    char* shm_ptr = (char*)shmat(shm_id, NULL, 0);
    return shm_ptr;
}

void shm_cleanup(int shm_id, char* shm_ptr) {
    if (shmdt(shm_ptr) == -1) {
        std::cerr << "Failed to detach shared memory" << std::endl;
    }
}

// Read messages from shared memory
void check_messages(char* shm_ptr) {
    std::string current_message(shm_ptr);

    if (!current_message.empty() && current_message != last_message) {
        // Parse message "sender: text"
        size_t colon_pos = current_message.find(": ");

        if (colon_pos != std::string::npos) {
            Message msg;
            msg.sender = current_message.substr(0, colon_pos);
            msg.text = current_message.substr(colon_pos + 2);
            msg.is_mine = (msg.sender == my_username);

            chat_messages.push_back(msg);
            last_message = current_message;
        }
    }
}

// Send message to shared memory
void send_message(char* shm_ptr, const std::string& message) {
    if (message.empty()) return;

    std::string full_message = my_username + ": " + message;
    strncpy(shm_ptr, full_message.c_str(), SHM_SIZE - 1);
    shm_ptr[SHM_SIZE - 1] = '\0';

    // Add to our own messages
    Message msg;
    msg.sender = my_username;
    msg.text = message;
    msg.is_mine = true;
    chat_messages.push_back(msg);

    last_message = full_message;
}

int main(int argc, char* argv[]) {
    // Get username
    if (argc > 1) {
        my_username = argv[1];
    } else {
        my_username = "User";
    }

    // Setup shared memory
    key_t shm_key = get_key();
    int shm_id = share_memory(shm_key);
    if (shm_id == -1) {
        return 1;
    }
    char* shm_ptr = shm_access(shm_id);

    // Window setup
    const int screenWidth = 700;
    const int screenHeight = 500;

    InitWindow(screenWidth, screenHeight, TextFormat("Chat - %s", my_username.c_str()));
    SetTargetFPS(60);

    char message_input[256] = "";
    bool message_edit_mode = false;

    while (!WindowShouldClose()) {
        // Check for new messages
        check_messages(shm_ptr);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Top toolbar
        GuiPanel((Rectangle){ 0, 0, screenWidth, 50 }, NULL);
        GuiLabel((Rectangle){ 20, 10, 400, 30 }, TextFormat("Logged in as: %s", my_username.c_str()));

        // Chat area background
        Rectangle chat_area = { 20, 70, screenWidth - 40, 360 };
        DrawRectangle(chat_area.x, chat_area.y, chat_area.width, chat_area.height, (Color){240, 240, 240, 255});
        DrawRectangleLines(chat_area.x, chat_area.y, chat_area.width, chat_area.height, DARKGRAY);

        // Handle scrolling
        float mouse_wheel = GetMouseWheelMove();
        if (mouse_wheel != 0) {
            scroll_offset -= mouse_wheel * 20;
            if (scroll_offset < 0) scroll_offset = 0;
        }

        // Clip messages to chat area
        BeginScissorMode(chat_area.x, chat_area.y, chat_area.width, chat_area.height);

        // Draw messages
        int y_pos = chat_area.y + 10 - (int)scroll_offset;
        int line_height = 20;

        for (size_t i = 0; i < chat_messages.size(); i++) {
            const Message& msg = chat_messages[i];

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
        GuiLabel((Rectangle){ 20, 440, 200, 20 }, "Type your message:");

        if (GuiTextBox((Rectangle){ 20, 465, screenWidth - 150, 30 },
                      message_input, 256, message_edit_mode)) {
            message_edit_mode = !message_edit_mode;
        }

        // Send button
        if (GuiButton((Rectangle){ screenWidth - 120, 465, 100, 30 }, "Send") ||
            (message_edit_mode && IsKeyPressed(KEY_ENTER))) {
            send_message(shm_ptr, message_input);
            memset(message_input, 0, sizeof(message_input));
            message_edit_mode = false;
        }

        EndDrawing();
    }

    // Cleanup
    shm_cleanup(shm_id, shm_ptr);
    CloseWindow();

    return 0;
}
