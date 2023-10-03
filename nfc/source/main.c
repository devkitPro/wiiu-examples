#include <nfc/nfc.h>
#include <coreinit/alarm.h>
#include <coreinit/debug.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <sysapp/switch.h>

#include <whb/proc.h>
#include <whb/log.h>
#include <whb/log_console.h>

#include <string.h>

static uint8_t *TLV_Parse(uint8_t *tlv, uint8_t *outTag, uint16_t *outLen)
{
   uint8_t tag = *tlv++;
   uint16_t len = *tlv++;

   // If the length is 0xFF, 2 bytes with length follow
   if (len == 0xFF) {
      len = ((uint16_t)tlv[0] << 8) | tlv[1];
      tlv += 2;
   }

   if (outTag) {
      *outTag = tag;
   }

   if (outLen) {
      *outLen = len;
   }

   // Return pointer to value
   return tlv;
}

static uint8_t *NDEF_GetType(uint8_t *rec, uint8_t *outTNF, uint8_t *outTypeLen)
{
   uint8_t recHdr = *rec++;
   uint8_t typeLen = *rec++;

   // Skip payload length
   if (recHdr & 0x10) {
      rec += 1;
   } else {
      rec += 4;
   }

   // Skip ID length, if it exists
   if (recHdr & 0x08) {
      rec += 1;
   }

   if (outTNF) {
      *outTNF = recHdr & 0x07;
   }

   if (outTypeLen) {
      *outTypeLen = typeLen;
   }

   // Return pointer to type field
   return rec;
}

static uint8_t *NDEF_GetPayload(uint8_t *rec, uint32_t* outPayloadLen)
{
   uint8_t recHdr = *rec++;
   uint8_t typeLen = *rec++;

   // Payload length can be 1 or 4 bytes long, depending on the SR bit
   uint32_t payloadLen;
   if (recHdr & 0x10) {
      payloadLen = *rec++;
   } else {
      payloadLen = ((uint32_t)rec[0] << 24) | ((uint32_t)rec[1] << 16) | ((uint32_t)rec[2] << 8) | rec[3];
      rec += 4;
   }

   // Read ID length, if it exists
   uint8_t idLen;
   if (recHdr & 0x08) {
      idLen = *rec++;
   } else {
      idLen = 0;
   }

   if (outPayloadLen) {
      *outPayloadLen = payloadLen;
   }

   // Return pointer to payload
   return rec + idLen + typeLen;
}

static void ndefReadCallback(VPADChan chan, NFCError error, uint32_t responseSize, void *responseData, void *userContext)
{
   if (error != 0) {
      WHBLogPrintf("Failed to read NDEF message\n");
      return;
   }

   // Parse the TLV again to get the V field (NDEF record)
   uint8_t *rec = TLV_Parse((uint8_t *)responseData, NULL, NULL);

   // Get the NDEF record type
   uint8_t tnf, typeLen;
   uint8_t *type = NDEF_GetType(rec, &tnf, &typeLen);

   // We only support well-known types
   if (tnf != 0x01) {
      WHBLogPrintf("Unsupported TNF\n");
      return;
   }

   // Make sure this is a URI
   if ((typeLen != 0x01) || (*type != 'U')) {
      WHBLogPrintf("Unsupported type\n");
      return;
   }

   // Get the NDEF payload
   uint32_t payloadLen;
   uint8_t *payload = NDEF_GetPayload(rec, &payloadLen);

   // We can now parse the payload
   uint8_t identifier = *payload++;

   char url[512];

   // Add URI identifier
   if (identifier == 0) {
      // N/A
   } else if (identifier == 1) {
      strcpy(url, "http://www.");
   } else if (identifier == 2) {
      strcpy(url, "https://www.");
   } else if (identifier == 3) {
      strcpy(url, "http://");
   } else if (identifier == 4) {
      strcpy(url, "https://");
   } else {
      WHBLogPrintf("Unssuported URI identifier code\n");
      return;
   }

   // Append the rest of the payload
   strncat(url, (char *)payload, payloadLen - 1);

   WHBLogPrintf("Opening: %s\n", url);

   // Open the browser overlay with the URI
   SysAppBrowserArgs args = { 0 };
   args.url = url;
   args.urlSize = strlen(url);
   SYSSwitchToBrowserForViewer(&args);
}

static void ndefDetectionCallback(VPADChan chan, NFCError error, uint32_t responseSize, void *responseData, void *userContext)
{
   if (error != 0 || responseSize < 16) {
      WHBLogPrintf("Failed to read capability container: %x %u\n", error, responseSize);
      return;
   }

   // Verify that the CC contains the NFC defined magic and contains a supported version
   uint8_t *cc = (uint8_t *)responseData;
   if (cc[0] != 0xE1 || cc[1] >> 4 != 0x1) {
      WHBLogPrintf("Tag not supported\n");
      return;
   }

   // Parse TLV
   uint8_t tag;
   uint16_t length;
   TLV_Parse((uint8_t *)responseData + 4, &tag, &length);

   // Check the tag field for the NDEF type
   if (tag != 0x03) {
      WHBLogPrintf("Tag does not contain an NDEF message\n");
      return;
   }

   uint8_t blockCount = (length + 4) / 4;

   // Read the full NDEF data using the NTAG FAST_READ command
   uint8_t command[3];
   command[0] = 0x3A; // Command (FAST_READ)
   command[1] = 0x04; // Start Block (04)
   command[2] = 0x04 + blockCount; // End Block (04 + block count)
   NFCError err = NFCSendRawData(VPAD_CHAN_0, FALSE, 500, 500,
      sizeof(command), blockCount * 4, command,
      ndefReadCallback, NULL);

   if (err != 0) {
      WHBLogPrintf("NFCSendRawData(1) failed: %x\n", err);
   }
}

static void getTagInfoCallback(VPADChan chan, NFCError error, NFCTagInfo *tagInfo, void *userContext)
{
   if (error != 0) {
      return;
   }

   WHBLogPrintf("Found tag!\n");
   WHBLogPrintf("  UID: %02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
      tagInfo->uid[0], tagInfo->uid[1], tagInfo->uid[2], tagInfo->uid[3],
      tagInfo->uid[4], tagInfo->uid[5], tagInfo->uid[6]);
   WHBLogPrintf("  Protocol: %d Technology: %d\n", tagInfo->protocol, tagInfo->technology);

   if (tagInfo->protocol == NFC_PROTOCOL_T2T) {
      // Read the first 16-bytes from the tag for NDEF detection
      uint8_t command[2];
      command[0] = 0x30; // Command (READ)
      command[1] = 0x03; // Block Number (03 = Capability Container)
      NFCError err = NFCSendRawDataWithPrePollingEx(VPAD_CHAN_0, TRUE, 500, 500,
         sizeof(command), 16, command,
         ndefDetectionCallback, NULL);
      if (err != 0) {
         WHBLogPrintf("NFCSendRawData(0) failed: %x\n", err);
      }
   }
}

static void alarmCallback(OSAlarm *, OSContext *)
{
   // Start searching for tags by getting their info
   NFCGetTagInfo(0, 500, getTagInfoCallback, NULL);
}

int main(int argc, char **argv)
{
   WHBProcInit();
   WHBLogConsoleInit();

   if (NFCInit(VPAD_CHAN_0) != 0) {
      OSFatal("Failed to start NFC initialization!");
      return -1;
   }

   // Wait for NFC to be ready
   OSTime startTime = OSGetTime();
   while (!NFCIsInit(VPAD_CHAN_0) && (OSGetTime() - startTime) < OSSecondsToTicks(2)) {
      NFCProc(VPAD_CHAN_0);
      OSYieldThread();
   }
   NFCProc(VPAD_CHAN_0);

   // Check if we reached the timeout
   if (!NFCIsInit(VPAD_CHAN_0)) {
      OSFatal("Failed to initialize NFC!");
      return -1;
   }

   // Create a periodic alarm to search for tags
   OSAlarm alarm;
   OSCreateAlarm(&alarm);
   OSSetPeriodicAlarm(&alarm, 0, OSSecondsToTicks(2), alarmCallback);

   WHBLogPrintf("Waiting for tag...\n");

   // Main loop, everything else is callback driven
   while (WHBProcIsRunning()) {
      NFCProc(VPAD_CHAN_0);
      WHBLogConsoleDraw();
   }

   OSCancelAlarm(&alarm);
   NFCShutdown(VPAD_CHAN_0);

   WHBLogConsoleFree();
   WHBProcShutdown();
   return 0;
}
