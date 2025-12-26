#include "Arduino.h"
#include <esp_task_wdt.h>
#include "Thread.h"
#include "Thermostat.h"
#include "log.h"

using namespace CLASSICDIY;

Thermostat *_thermostat = new Thermostat();
ThreadController _controller = ThreadController();
Thread *_workerThreadTFT = new Thread();
Thread *_workerThreadHeat = new Thread();

void setup()
{
   // wait for Serial to connect, give up after 5 seconds, USB may not be connected
   delay(3000);
   unsigned long start = millis();
   Serial.begin(115200);
   while (!Serial)
   {
      if (5000 < millis() - start)
      {
         break;
      }
   }

   logd("------------ESP32 specifications ---------------");
   logd("Chip Model: %s", ESP.getChipModel());
   logd("Chip Revision: %d", ESP.getChipRevision());
   logd("Number of CPU Cores: %d", ESP.getChipCores());
   logd("CPU Frequency: %d MHz", ESP.getCpuFreqMHz());
   logd("Flash Memory Size: %d MB", ESP.getFlashChipSize() / (1024 * 1024));
   logd("Flash Frequency: %d MHz", ESP.getFlashChipSpeed() / 1000000);
   logd("Heap Size: %d KB", ESP.getHeapSize() / 1024);
   logd("Free Heap: %d KB", ESP.getFreeHeap() / 1024);
   logd("------------ESP32 specifications ---------------");
   _workerThreadTFT->onRun([]() { _thermostat->runTFT(); });
   _workerThreadTFT->setInterval(250);
   _controller.add(_workerThreadTFT);
   _workerThreadHeat->onRun([]() { _thermostat->runHeater(); });
   _workerThreadHeat->setInterval(1000);
   _controller.add(_workerThreadHeat);
   esp_task_wdt_init(60, true); // 60-second timeout, panic on timeout
   esp_task_wdt_add(NULL);
   _thermostat->Setup();
   logd("Free Heap after setup: %d KB", ESP.getFreeHeap() / 1024);
   logd("------------Setup Done ---------------");
}

void loop()
{
   _controller.run();
   esp_task_wdt_reset(); // feed watchdog
   delay(10);
}