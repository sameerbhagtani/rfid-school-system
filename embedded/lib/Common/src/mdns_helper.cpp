#include "mdns_helper.h"

#include <ESP8266mDNS.h>

bool initMDNS(const char *hostname)
{
    return MDNS.begin(hostname);
}

void updateMDNS()
{
    MDNS.update();
}
