/*
 * Original license:
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 *
 * Norbert Koszó <k.norbi@gmail.com> has messed it up a little bit. Jeroen
 * Domburg still deserves the beer.
 */

#include <esp8266.h>
#include "httpd.h"
#include "httpdespfs.h"
#include "cgiflash.h"
#include "auth.h"
#include "espfs.h"
#include "captdns.h"
#include "webpages-espfs.h"
#include "cgiwebsocket.h"
#include "stdout.h"
#include "wifi-config-module/src/wifi-config-module.h"

HttpdBuiltInUrl* copyHttpdBuiltInUrls(const HttpdBuiltInUrl * source,
                          HttpdBuiltInUrl *target);
#ifdef ESPFS_POS
CgiUploadFlashDef uploadParams={
    .type=CGIFLASH_TYPE_ESPFS,
    .fw1Pos=ESPFS_POS,
    .fw2Pos=0,
    .fwSize=ESPFS_SIZE,
};
#define INCLUDE_FLASH_FNS
#endif
#ifdef OTA_FLASH_SIZE_K
CgiUploadFlashDef uploadParams={
    .type=CGIFLASH_TYPE_FW,
    .fw1Pos=0x1000,
    .fw2Pos=((OTA_FLASH_SIZE_K*1024)/2)+0x1000,
    .fwSize=((OTA_FLASH_SIZE_K*1024)/2)-0x1000,
    .tagName=OTA_TAGNAME
};
#define INCLUDE_FLASH_FNS
#endif

/*
This is the main url->function dispatching data struct.
In short, it's a struct with various URLs plus their handlers. The handlers can
be 'standard' CGI functions you wrote, or 'special' CGIs requiring an argument.
They can also be auth-functions. An asterisk will match any url starting with
everything before the asterisks; "*" matches everything. The list will be
handled top-down, so make sure to put more specific rules above the more
general ones. Authorization things (like authBasic) act as a 'barrier' and
should be placed above the URLs they protect.
*/
HttpdBuiltInUrl builtInUrlsListBegin[] = {
    {"*", cgiRedirectApClientToHostname, "esp8266.nonet"},
    {"/kuku", cgiRedirect, "/index.html"},
#ifdef INCLUDE_FLASH_FNS
    {"/flash/next", cgiGetFirmwareNext, &uploadParams},
    {"/flash/upload", cgiUploadFirmware, &uploadParams},
#endif
    {"/flash/reboot", cgiRebootFirmware, NULL}
};
const size_t builtInUrlsListBeginSize = sizeof(builtInUrlsListBegin) / sizeof(builtInUrlsListBegin[0]);

HttpdBuiltInUrl builtInUrlsListEnd[] = {
    {"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
    {NULL, NULL, NULL}
};
const size_t builtInUrlsListEndSize =
        sizeof(builtInUrlsListEnd) / sizeof(builtInUrlsListEnd[0]);

HttpdBuiltInUrl *allUrls;

void concatenateBuiltInUrls()
{
    size_t requiredSize = 0;
    requiredSize += builtInUrlsListBeginSize;
    requiredSize += wifi_module_builtInUrlsSize;
    requiredSize += builtInUrlsListEndSize;
    os_printf("requiredSize = %d\n\r", requiredSize);

    allUrls = (HttpdBuiltInUrl*) malloc(requiredSize * sizeof(builtInUrlsListBegin[0]));
    HttpdBuiltInUrl *u = allUrls;
    memcpy(u, builtInUrlsListBegin, builtInUrlsListBeginSize * sizeof(builtInUrlsListBegin[0]));
    u += builtInUrlsListBeginSize;
    memcpy(u, wifi_module_builtInUrls,
           wifi_module_builtInUrlsSize * sizeof(wifi_module_builtInUrls[0]));
    u += wifi_module_builtInUrlsSize;
    memcpy(u, builtInUrlsListEnd,
           builtInUrlsListEndSize * sizeof(builtInUrlsListEnd[0]));

    size_t i;
    for (i = 0; i < requiredSize; ++i)
        os_printf("%d: %s %p %p\n\r", i, allUrls[i].url, (void*)allUrls[i].cgiCb, (const char*)allUrls[i].cgiArg);
}

//Main routine. Initialize stdout, the I/O, filesystem and the webserver and we're done.
void user_init(void) {
    captdnsInit();
    stdoutInit();
    os_printf("\n\rPrintf initialized\n\r");
    // 0x40200000 is the base address for spi flash memory mapping, ESPFS_POS is the position
    // where image is written in flash that is defined in Makefile.
#ifdef ESPFS_POS
    espFsInit((void*)(0x40200000 + ESPFS_POS));
#else
    espFsInit((void*)(webpages_espfs_start));
#endif
    concatenateBuiltInUrls();

    httpdInit(allUrls, 80);
    os_printf("READY\n\r");
}

void user_rf_pre_init() {
    //Not needed, but some SDK versions want this defined.
}
