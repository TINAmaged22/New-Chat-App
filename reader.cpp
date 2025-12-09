#include <iostream>
#include <cstring>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define SHM_SIZE 1024
#define SHM_KEY 1234

int main() {
    // Access shared memory
    int shmid = shmget(SHM_KEY, SHM_SIZE, 0666 | IPC_CREAT);
    if (shmid == -1) {
        std::cerr << "Failed to access shared memory" << std::endl;
        return 1;
    }

    // Attach to shared memory
    char* shm_ptr = (char*)shmat(shmid, NULL, 0);
    if (shm_ptr == (char*)-1) {
        std::cerr << "Failed to attach shared memory" << std::endl;
        return 1;
    }

    std::cout << "=== Chat Reader ===" << std::endl;
    std::cout << "Waiting for messages (Ctrl+C to exit)..." << std::endl;
    std::cout << std::endl;

    std::string last_message = "";

    while (true) {
        // Read message from shared memory
        std::string current_message(shm_ptr);

        // Only display if message has changed and is not empty
        if (!current_message.empty() && current_message != last_message) {
            std::cout << current_message << std::endl;
            last_message = current_message;
        }

        // Sleep for a bit to avoid busy waiting
        usleep(100000); // 100ms
    }

    // Detach from shared memory
    shmdt(shm_ptr);

    return 0;
}
