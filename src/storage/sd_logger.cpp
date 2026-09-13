#include "sd_logger.h"
#include "FS.h"
#include "SD_MMC.h"

static File logFile;

void initSDLogger() {
    // Delete old log
    if (SD_MMC.exists("/debug.log")) {
        SD_MMC.remove("/debug.log");
    }
    
    // Create new log
    logFile = SD_MMC.open("/debug.log", FILE_WRITE);
    if (logFile) {
        logFile.println("=== DEBUG LOG START ===");
        logFile.printf("Boot time: %lu ms\n", millis());
        logFile.flush();
    }
}

void logToSD(const char* msg) {
    if (!logFile) return;
    
    logFile.printf("[%lu] %s\n", millis(), msg);
    logFile.flush();  // CRITICAL: Flush immediately so data survives crashes
}

void closeSDLogger() {
    if (logFile) {
        logFile.println("=== LOG END ===");
        logFile.close();
    }
}