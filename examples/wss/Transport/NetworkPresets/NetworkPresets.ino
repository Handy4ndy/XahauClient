/**
 * XahauClient Example — NetworkPresets
 *
 * WSS network presets
 *
 * Demonstrates: Demonstrates Mainnet/Testnet network presets over WebSocket.
 * Hardware: ESP32 / ESP32-S3 (Wi-Fi + WebSocketsClient)
 * Network: Xahau Testnet by default (NetworkID 21338); set XAHAU_NETWORK=0 for Mainnet (21337).
 * Config: Set WIFI_SSID / WIFI_PASSWORD and any XAHAU_SEED placeholders before live use. Never commit real credentials.
 *
 * SPDX-License-Identifier: MIT
 */
#define XAHAU_FEATURE_NET 1
#include <WiFi.h>
#include <XahauClient.h>
#include <xahau/sketch/example_runtime.h>
#include <string.h>
// TLS + WebSockets + large JSON need more than the default ~8 KiB loopTask stack.
// Stack canary panics (Guru Meditation) on server_info/account_lines were caused by
// large on-stack response buffers + mbedTLS. Prefer BSS buffers (s_respBig) + 32 KiB stack.
#if defined(ESP_PLATFORM) && defined(SET_LOOP_TASK_STACK_SIZE)
SET_LOOP_TASK_STACK_SIZE(32 * 1024);
#endif

// ---------------------------------------------------------------------------
// CONFIG - edit like a .js / .py script
// ---------------------------------------------------------------------------
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// 0 = Mainnet, 1 = Testnet (default); 2+ also maps to Testnet (no public Devnet)
const int   XAHAU_NETWORK  = 1;

// Optional seed: if set, ACCOUNT below can be left empty to use derived address.
// Classic family seed sEd... or s...  (never commit mainnet keys)
const char* XAHAU_SEED     = "sEdPLACEHOLDER_REPLACE_ME";

// Classic address to query. Empty string "" => derive from XAHAU_SEED when possible.
const char* ACCOUNT       = "";

// Optional placeholders for hash / blob demos
const char* TX_HASH_HEX   = "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";
const char* TX_BLOB_HEX   = "12000022800000002400000001201B0000006461400000000000000168400000000000000C7321ED00";

// Print full JSON response (can be large - trim if Serial floods)
const bool  PRINT_RAW_JSON = true;

// Optional TLS pin for Mainnet production:
//   ws.setSslFingerprint("AA:BB:...");
//   ws.setRequireSslPin(true);
//   ws.setInsecureSsl(false);

static bool resolveAccount(char* out, size_t outCap, xahau::KeyPair* keysOut) {
  if (ACCOUNT && ACCOUNT[0]) {
    strncpy(out, ACCOUNT, outCap - 1);
    out[outCap - 1] = '\0';
    return true;
  }
  if (!XAHAU_SEED || !XAHAU_SEED[0] || strstr(XAHAU_SEED, "PLACEHOLDER")) {
    Serial.println(F("Set ACCOUNT or a valid XAHAU_SEED in CONFIG."));
    return false;
  }
  xahau::KeyPair keys;
  if (xahau::deriveKeysFromSeed(XAHAU_SEED, keys) != xahau::Status::Ok) {
    Serial.println(F("deriveKeysFromSeed failed."));
    return false;
  }
  strncpy(out, keys.address, outCap - 1);
  out[outCap - 1] = '\0';
  if (keysOut) *keysOut = keys;
  else xahau::secureClear(keys);
  return true;
}

// BSS globals — WebSocketsClient + response buffers must NOT live on setup() stack.
// Large stack locals (resp[8k] + TLS) caused loopTask stack-canary panics in ClientTest.
static xahau::WsXahauTransport s_ws;
static xahau::XahauRpcClient s_client(s_ws);
static char s_respBig[8192];
static char s_printTmp[1536];

static bool connectWs() {
  if (!xahau::sketch::wifiConnect(WIFI_SSID, WIFI_PASSWORD)) return false;
  const xahau::NetworkEndpoint& net = xahau::sketch::networkFromIndex(XAHAU_NETWORK);
  Serial.print(F("Network: "));
  Serial.print(net.name);
  Serial.print(F("  wss://"));
  Serial.print(net.wsHost);
  Serial.print(F(":"));
  Serial.println(net.wsPort);
  s_ws.setEndpoint(net);
  s_client.setResponseBuffer(s_respBig, sizeof(s_respBig));
  s_client.setTimeoutMs(20000);
  return true;
}

static void printStatus(const char* label, xahau::Status st) {
  Serial.print(label);
  Serial.print(F(": "));
  Serial.println(xahau::statusName(st));
}

static void printJsonSnippet(const char* json) {
  if (!PRINT_RAW_JSON || !json) return;
  Serial.println(F("--- JSON ---"));
  // Cap print length for Serial — use BSS temp (never stack) to avoid canary panics
  size_t n = strlen(json);
  if (n > 1500) {
    memcpy(s_printTmp, json, 1500);
    s_printTmp[1500] = '\0';
    Serial.println(s_printTmp);
    Serial.println(F("... (truncated)"));
  } else {
    Serial.println(json);
  }
  Serial.println(F("------------"));
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println();
  Serial.println(F("=== XahauClient WSS - network presets + WSS endpoint ==="));
  Serial.print(F("Library "));
  Serial.println(XAHAU_CLIENT_VERSION);

  if (!connectWs()) return;
  xahau::XahauRpcClient& client = s_client;

  // Demonstrate switching public endpoints (no account required).
  const xahau::NetworkEndpoint* nets[2] = {
      &xahau::kNetworkMainnet, &xahau::kNetworkTestnet};
  for (int i = 0; i < 2; ++i) {
    const xahau::NetworkEndpoint& ep = *nets[i];
    Serial.print(ep.name);
    Serial.print(F("  valid="));
    Serial.print(xahau::networkEndpointValid(ep) ? F("yes") : F("no"));
    Serial.print(F("  wss://"));
    Serial.print(ep.wsHost);
    Serial.print(F(":"));
    Serial.print(ep.wsPort);
    Serial.print(F("  rpc:"));
    Serial.println(ep.rpcPort);
  }
  Serial.print(F("allPublicNetworksWired: "));
  Serial.println(xahau::allPublicNetworksWired() ? F("yes") : F("no"));

  // Live ping on selected network
  xahau::Status st = client.ping();
  printStatus("ping on XAHAU_NETWORK", st);

  Serial.println(F("Done."));
}

void loop() {}
