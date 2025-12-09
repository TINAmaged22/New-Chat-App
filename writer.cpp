#include <iostream>
#include <cstring>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define SHM_SIZE 1024
#define SHM_KEY 1234

int main(int argc, char* argv[]) {
    std::string username;

    // Get username from command line or prompt
    if (argc > 1) {
        username = argv[1];
    } else {
        std::cout << "Enter your name: ";
        std::getline(std::cin, username);
        if (username.empty()) {
            username = "Anonymous";
        }
    }

    // Create shared memory
    int shmid = shmget(SHM_KEY, SHM_SIZE, 0666 | IPC_CREAT);
    if (shmid == -1) {
        std::cerr << "Failed to create shared memory" << std::endl;
        return 1;
    }

    // Attach to shared memory
    char* shm_ptr = (char*)shmat(shmid, NULL, 0);
    if (shm_ptr == (char*)-1) {
        std::cerr << "Failed to attach shared memory" << std::endl;
        return 1;
    }

    std::cout << "=== Chat Writer - " << username << " ===" << std::endl;
    std::cout << "Type your messages (type 'quit' to exit):" << std::endl;
    std::cout << std::endl;

    std::string message;
    while (true) {
        std::cout << "You: ";
        std::getline(std::cin, message);

        if (message == "quit") {
            break;
        }

        // Format: "username: message"
        std::string full_message = username + ": " + message;

        // Write message to shared memory
        strncpy(shm_ptr, full_message.c_str(), SHM_SIZE - 1);
        shm_ptr[SHM_SIZE - 1] = '\0';
    }

    // Detach from shared memory
    shmdt(shm_ptr);

    std::cout << "Writer exited." << std::endl;
    return 0;
}
