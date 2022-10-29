#include <nn/nfp.h>
#include <vpad/input.h>
#include <coreinit/debug.h>

#include <whb/proc.h>
#include <whb/log.h>
#include <whb/log_console.h>

static void PrintAmiiboInfo()
{
   // Mount the amiibo so data can be accessed
   if (!nn::nfp::Mount()) {
      WHBLogPrint("Failed to mount amiibo");
      return;
   }

   // Print out some of the common info
   nn::nfp::CommonInfo commonInfo;
   if (nn::nfp::GetNfpCommonInfo(&commonInfo)) {
      WHBLogPrintf("CharacterID: 0x%02x%02x%02x SeriesID: 0x%02x NumberingID: 0x%04x",
         commonInfo.characterID[0], commonInfo.characterID[1], commonInfo.characterID[2],
         commonInfo.seriesID, commonInfo.numberingID);
      WHBLogPrintf("Last write: %u-%u-%u Writes: %u",
         commonInfo.lastWriteDate.year, commonInfo.lastWriteDate.month, commonInfo.lastWriteDate.day,
         commonInfo.writes);
   } else {
      WHBLogPrint("Failed to get common info");
   }

   // Print out some of the register info
   nn::nfp::RegisterInfo registerInfo;
   if (nn::nfp::GetNfpRegisterInfo(&registerInfo)) {
      // Quick and dirty way to convert from UTF-16
      char name[11];
      for (size_t i = 0; i < 11; ++i)
         name[i] = registerInfo.name[i] & 0xff;

      // Quick and dirty way to convert from UTF-16-LE
      char owner[10];
      for (size_t i = 0; i < 10; ++i)
         owner[i] = registerInfo.mii.data.core.mii_name[i] >> 8;

      WHBLogPrintf("Name: \"%s\" Owner: \"%s\"", name, owner);
      WHBLogPrintf("Registered: %u-%u-%u",
         registerInfo.registerDate.year, registerInfo.registerDate.month, registerInfo.registerDate.day);
   } else {
      WHBLogPrint("Failed to get register info");
   }

   // Print some info about the application area if it exists
   if (nn::nfp::IsExistApplicationArea()) {
      nn::nfp::AdminInfo adminInfo;
      if (nn::nfp::GetNfpAdminInfo(&adminInfo)) {
         WHBLogPrintf("Title: %08x-%08x Access ID: %08x",
            (uint32_t) (adminInfo.titleID >> 32), (uint32_t) (adminInfo.titleID & 0xffffffff),
            adminInfo.accessID);

         // Print out some of the application data
         if (nn::nfp::OpenApplicationArea(adminInfo.accessID)) {
            uint32_t data[4];
            if (nn::nfp::ReadApplicationArea(&data, sizeof(data))) {
               WHBLogPrintf("Start of application data: %08x %08x %08x %08x",
                  data[0], data[1], data[2], data[3]);
            }
         }
      } else {
         WHBLogPrint("Failed to get admin info");
      }
   } else {
      WHBLogPrint("Application Area doesn't exist");
   }

   // Unmount the tag
   nn::nfp::Unmount();
}

int main(int argc, char **argv)
{
   WHBProcInit();
   WHBLogConsoleInit();

   // Initialize nn::nfp
   if (!nn::nfp::Initialize()) {
      OSFatal("Failed to initialize nn::nfp");
      return -1;
   }

   // Start searching for amiibo
   WHBLogPrint("Searching for amiibo...");
   nn::nfp::StartDetection();

   bool removed = true;
   while (WHBProcIsRunning()) {
      // Get the current state
      nn::nfp::NfpState state = nn::nfp::GetNfpState();

      // Check if a tag was found
      // Alternatively an event to wait for could be set using 'nn::nfp::SetActivateEvent'
      if (state == nn::nfp::NfpState::Found && removed) {
         WHBLogPrint("Amiibo found");
         PrintAmiiboInfo();

         // Wait for the tag to be removed
         WHBLogPrint("Waiting for the amiibo to be removed...");
         removed = false;
      }

      // If the tag was removed, restart detection
      // Alternatively an event to wait for removal could be set using 'nn::nfp::SetDeactivateEvent'
      if (state == nn::nfp::NfpState::Removed) {
         WHBLogPrint("Amiibo removed, restarting detection...");
         removed = true;
         nn::nfp::StartDetection();
      }

      WHBLogConsoleDraw();
   }

   nn::nfp::Finalize();

   WHBLogConsoleFree();
   WHBProcShutdown();
   return 0;
}
