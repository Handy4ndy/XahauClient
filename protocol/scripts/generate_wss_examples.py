#!/usr/bin/env python3
"""
Generate ESP32 *live WSS* examples (JSON-RPC over WebSocket).

Offline builders: examples/rpc/ (no Wi-Fi).

Layout mirrors Xahau public API areas + library client helpers:
  examples/wss/
    Server/          ping, fee, server_info, server_state, ledger*
    Account/         account_info, lines, tx, objects, sequence helpers
    Transaction/     submit, submit fail_hard, tx lookup, multisign envelope
    Objects/         vault / loan ID fetch helpers
    Transport/       rawRequest, end-to-end prepare+submit
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
RPC = ROOT / "examples" / "wss"

COMMON_HDR = '''/**
 * ============================================================================
 * XahauClient WSS Example - {title}
 * ============================================================================
 *
 * Command / API: {api}
 * Group: {group}
 *
 * What this sketch does
 * ---------------------
 * Connects over WebSocket (WSS) to a public Xahau cluster using library network
 * presets, builds a JSON-RPC request (or uses XahauRpcClient helpers), and prints
 * the response on Serial.
 *
 * Library surface
 * ---------------
 * {lib_surface}
 *
 * Xahau docs
 * ---------
 * Public methods: https://Xahau.org/docs/references/http-websocket-apis/public-api-methods
 * Networks:       library docs/NETWORKS.md
 * Security:       library docs/SECURITY.md (TLS pin optional)
 *
 * Board: ESP32 + WiFi + WebSocketsClient
 * Safety: never commit real WiFi passwords or mainnet seeds.
 */
'''

CONFIG = '''
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

// 0 = Mainnet, 1 = Testnet (default), 2 = Devnet
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
'''

HELPERS = '''
static bool resolveAccount(char* out, size_t outCap, xahau::KeyPair* keysOut) {
  if (ACCOUNT && ACCOUNT[0]) {
    strncpy(out, ACCOUNT, outCap - 1);
    out[outCap - 1] = '\\0';
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
  out[outCap - 1] = '\\0';
  if (keysOut) *keysOut = keys;
  else xahau::secureClear(keys);
  return true;
}

// BSS globals - WebSocketsClient + response buffers must NOT live on setup() stack.
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
  // Cap print length for Serial - use BSS temp (never stack) to avoid canary panics
  size_t n = strlen(json);
  if (n > 1500) {
    memcpy(s_printTmp, json, 1500);
    s_printTmp[1500] = '\\0';
    Serial.println(s_printTmp);
    Serial.println(F("... (truncated)"));
  } else {
    Serial.println(json);
  }
  Serial.println(F("------------"));
}
'''


def sketch(title, api, group, lib_surface, body):
    return (
        COMMON_HDR.format(title=title, api=api, group=group, lib_surface=lib_surface)
        + CONFIG
        + HELPERS
        + "\nvoid setup() {\n"
        + "  Serial.begin(115200);\n  delay(1500);\n  Serial.println();\n"
        + f'  Serial.println(F("=== XahauClient WSS - {title} ==="));\n'
        + "  Serial.print(F(\"Library \"));\n  Serial.println(XAHAU_CLIENT_VERSION);\n\n"
        + "  if (!connectWs()) return;\n"
        + "  xahau::XahauRpcClient& client = s_client;\n\n"
        + body
        + "\n  Serial.println(F(\"Done.\"));\n}\n\nvoid loop() {}\n"
    )


EXAMPLES = []


def add(group, name, title, api, lib_surface, body):
    EXAMPLES.append(dict(group=group, name=name, title=title, api=api, lib=lib_surface, body=body))


# ---- Server ----
add(
    "Server",
    "Ping",
    "ping",
    "ping",
    "XahauRpcClient::ping()  |  rpcBuildPing",
    """  xahau::Status st = client.ping();
  printStatus("ping", st);
  if (st == xahau::Status::Ok) Serial.println(F("Server responded (no error field)."));
""",
)

add(
    "Server",
    "Fee",
    "fee",
    "fee",
    "XahauRpcClient::fetchFeeDrops()  |  rpcBuildFee / rpcParseFeeDrops",
    """  uint64_t fee = 0;
  xahau::Status st = client.fetchFeeDrops(fee);
  printStatus("fetchFeeDrops", st);
  Serial.print(F("base fee (drops): "));
  Serial.println((unsigned long)fee);

  // Also show raw builder path
  char req[64];
  st = xahau::rpcBuildFee(req, sizeof(req));
  printStatus("rpcBuildFee", st);
  st = client.rawRequest(req, s_respBig, sizeof(s_respBig));
  printStatus("raw fee", st);
  printJsonSnippet(s_respBig);
""",
)

add(
    "Server",
    "ServerInfo",
    "server_info",
    "server_info",
    "rpcBuildServerInfo + XahauRpcClient::rawRequest",
    """  char req[64];
  xahau::Status st = xahau::rpcBuildServerInfo(req, sizeof(req));
  printStatus("rpcBuildServerInfo", st);
  st = client.rawRequest(req, s_respBig, sizeof(s_respBig));
  printStatus("server_info", st);
  printJsonSnippet(s_respBig);
  uint32_t led = 0;
  if (xahau::rpcParseLedgerIndex(s_respBig, led) == xahau::Status::Ok) {
    Serial.print(F("parsed ledger_index: "));
    Serial.println(led);
  }
""",
)

add(
    "Server",
    "ServerState",
    "server_state",
    "server_state",
    "rpcBuildServerState + rawRequest",
    """  char req[64];
  xahau::Status st = xahau::rpcBuildServerState(req, sizeof(req));
  printStatus("rpcBuildServerState", st);
  st = client.rawRequest(req, s_respBig, sizeof(s_respBig));
  printStatus("server_state", st);
  printJsonSnippet(s_respBig);
""",
)

add(
    "Server",
    "LedgerClosed",
    "ledger_closed",
    "ledger_closed",
    "rpcBuildLedgerClosed + rawRequest / rpcParseLedgerIndex",
    """  char req[64];
  xahau::Status st = xahau::rpcBuildLedgerClosed(req, sizeof(req));
  printStatus("rpcBuildLedgerClosed", st);
  st = client.rawRequest(req, s_respBig, sizeof(s_respBig));
  printStatus("ledger_closed", st);
  printJsonSnippet(s_respBig);
  uint32_t led = 0;
  if (xahau::rpcParseLedgerIndex(s_respBig, led) == xahau::Status::Ok) {
    Serial.print(F("closed ledger: "));
    Serial.println(led);
  }
""",
)

add(
    "Server",
    "Ledger",
    "ledger",
    "ledger",
    "rpcBuildLedger(ledger_index, transactions)",
    """  char req[160];
  // "validated" is allowlisted; transactions=false keeps payload smaller
  xahau::Status st = xahau::rpcBuildLedger("validated", false, req, sizeof(req));
  printStatus("rpcBuildLedger", st);
  st = client.rawRequest(req, s_respBig, sizeof(s_respBig));
  printStatus("ledger", st);
  printJsonSnippet(s_respBig);
""",
)

add(
    "Server",
    "FeeAndLedger",
    "fee + ledger (client helper)",
    "fee / ledger_closed (composite)",
    "XahauRpcClient::fetchFeeAndLedger()",
    """  xahau::FeeAndLedger fl;
  xahau::Status st = client.fetchFeeAndLedger(fl);
  printStatus("fetchFeeAndLedger", st);
  Serial.print(F("feeDrops: "));
  Serial.println((unsigned long)fl.feeDrops);
  Serial.print(F("ledgerIndex: "));
  Serial.println(fl.ledgerIndex);
  Serial.print(F("suggested LLS: "));
  Serial.println(fl.lastLedgerSequence);
""",
)

# ---- Account ----
add(
    "Account",
    "AccountInfo",
    "account_info",
    "account_info",
    "XahauRpcClient::fetchAccountInfo()  |  rpcBuildAccountInfo / rpcParseAccountInfoSnapshot",
    """  char acct[48];
  if (!resolveAccount(acct, sizeof(acct), nullptr)) return;

  xahau::AccountInfoSnapshot snap;
  xahau::Status st = client.fetchAccountInfo(acct, snap);
  printStatus("fetchAccountInfo", st);
  Serial.print(F("account: "));
  Serial.println(acct);
  Serial.print(F("found: "));
  Serial.println(snap.found ? F("yes") : F("no"));
  if (snap.found) {
    Serial.print(F("sequence: "));
    Serial.println(snap.sequence);
    Serial.print(F("balance drops: "));
    Serial.println((unsigned long)snap.balanceDrops);
    Serial.print(F("ownerCount: "));
    Serial.println(snap.ownerCount);
    Serial.print(F("ledgerIndex: "));
    Serial.println(snap.ledgerIndex);
  }

  char req[192];
  st = xahau::rpcBuildAccountInfo(acct, req, sizeof(req));
  printStatus("rpcBuildAccountInfo", st);
  st = client.rawRequest(req, s_respBig, sizeof(s_respBig));
  printStatus("raw account_info", st);
  printJsonSnippet(s_respBig);
""",
)

add(
    "Account",
    "AccountLines",
    "account_lines",
    "account_lines",
    "rpcBuildAccountLines + rawRequest",
    """  char acct[48];
  if (!resolveAccount(acct, sizeof(acct), nullptr)) return;

  char req[192];
  xahau::Status st = xahau::rpcBuildAccountLines(acct, req, sizeof(req));
  printStatus("rpcBuildAccountLines", st);
  st = client.rawRequest(req, s_respBig, sizeof(s_respBig));
  printStatus("account_lines", st);
  printJsonSnippet(s_respBig);
""",
)

add(
    "Account",
    "AccountTx",
    "account_tx",
    "account_tx",
    "rpcBuildAccountTx + rawRequest",
    """  char acct[48];
  if (!resolveAccount(acct, sizeof(acct), nullptr)) return;

  char req[256];
  xahau::Status st = xahau::rpcBuildAccountTx(acct, 5, req, sizeof(req));
  printStatus("rpcBuildAccountTx", st);
  st = client.rawRequest(req, s_respBig, sizeof(s_respBig));
  printStatus("account_tx", st);
  printJsonSnippet(s_respBig);
""",
)

add(
    "Account",
    "AccountObjects",
    "account_objects",
    "account_objects",
    "rpcBuildAccountObjects + XahauRpcClient::fetchAccountObjectIds",
    """  char acct[48];
  if (!resolveAccount(acct, sizeof(acct), nullptr)) return;

  // All objects - use BSS s_respBig (not stack) to avoid loopTask stack canary panics
  char req[256];
  xahau::Status st = xahau::rpcBuildAccountObjects(acct, nullptr, req, sizeof(req));
  printStatus("rpcBuildAccountObjects(all)", st);
  st = client.rawRequest(req, s_respBig, sizeof(s_respBig));
  printStatus("account_objects", st);
  printJsonSnippet(s_respBig);

  // Typed filter example (allowlisted: vault, state, ...)
  st = xahau::rpcBuildAccountObjects(acct, "state", req, sizeof(req));
  printStatus("rpcBuildAccountObjects(state)", st);
  st = client.rawRequest(req, s_respBig, sizeof(s_respBig));
  printStatus("account_objects state", st);

  uint8_t ids[8][32];
  uint8_t n = 0;
  st = client.fetchAccountObjectIds(acct, nullptr, ids, 8, n);
  printStatus("fetchAccountObjectIds", st);
  Serial.print(F("object count (max 8): "));
  Serial.println(n);
""",
)

add(
    "Account",
    "FetchSequenceAndLls",
    "account_info (sequence + LLS helper)",
    "account_info (+ ledger for LLS)",
    "XahauRpcClient::fetchSequenceAndLls()",
    """  char acct[48];
  if (!resolveAccount(acct, sizeof(acct), nullptr)) return;

  uint32_t seq = 0, lls = 0;
  xahau::Status st = client.fetchSequenceAndLls(acct, seq, lls);
  printStatus("fetchSequenceAndLls", st);
  Serial.print(F("Sequence: "));
  Serial.println(seq);
  Serial.print(F("LastLedgerSequence: "));
  Serial.println(lls);
  Serial.println(F("Use these when building/signing a transaction bag."));
""",
)

# ---- Transaction ----
add(
    "Transaction",
    "TxLookup",
    "tx",
    "tx",
    "rpcBuildTx + rawRequest",
    """  // Replace TX_HASH_HEX with a real 64-char hex hash from an explorer.
  char req[160];
  xahau::Status st = xahau::rpcBuildTx(TX_HASH_HEX, false, req, sizeof(req));
  printStatus("rpcBuildTx", st);
  if (st != xahau::Status::Ok) {
    Serial.println(F("TX_HASH_HEX must be even-length hex (64 chars for hash)."));
    return;
  }
  st = client.rawRequest(req, s_respBig, sizeof(s_respBig));
  printStatus("tx", st);
  printJsonSnippet(s_respBig);
""",
)

add(
    "Transaction",
    "SubmitBlob",
    "submit",
    "submit",
    "XahauRpcClient::submitBlob()  |  rpcBuildSubmit",
    """  // WARNING: submitting TX_BLOB_HEX as-is will almost always fail (placeholder).
  // Prefer Transport/EndToEndPrepareSubmit which signs a live Payment first.
  char req[2200];
  xahau::Status st = xahau::rpcBuildSubmit(TX_BLOB_HEX, req, sizeof(req));
  printStatus("rpcBuildSubmit", st);
  if (st != xahau::Status::Ok) {
    Serial.println(F("TX_BLOB_HEX must be even-length hex."));
    return;
  }

  xahau::SubmitResult sr;
  st = client.submitBlob(TX_BLOB_HEX, sr);
  printStatus("submitBlob", st);
  Serial.print(F("engine_result: "));
  Serial.println(sr.engineResult);
  Serial.print(F("hash: "));
  Serial.println(sr.txHash);
  Serial.println(sr.tesSuccess ? F("tesSUCCESS") : F("not tesSUCCESS (expected for dummy blob)"));
""",
)

add(
    "Transaction",
    "SubmitFailHard",
    "submit (fail_hard)",
    "submit + fail_hard",
    "XahauRpcClient::submitBlobFailHard()  |  rpcBuildSubmitEx",
    """  char req[2200];
  xahau::Status st = xahau::rpcBuildSubmitEx(TX_BLOB_HEX, true, req, sizeof(req));
  printStatus("rpcBuildSubmitEx(fail_hard)", st);

  xahau::SubmitResult sr;
  st = client.submitBlobFailHard(TX_BLOB_HEX, sr);
  printStatus("submitBlobFailHard", st);
  Serial.print(F("engine_result: "));
  Serial.println(sr.engineResult);
""",
)

add(
    "Transaction",
    "SubmitMultisigned",
    "submit_multisigned",
    "submit_multisigned",
    "rpcBuildSubmitMultisigned (caller supplies signed tx_json object)",
    """  // Multisign body must be a JSON object the wallet already assembled.
  // This demo only shows the builder validation + envelope; replace with real tx_json.
  const char* txJson =
      "{\\"TransactionType\\":\\"Payment\\",\\"Account\\":\\"rPLACEHOLDER\\",\\"Destination\\":\\"rPLACEHOLDER\\"}";
  char req[2048];
  xahau::Status st = xahau::rpcBuildSubmitMultisigned(txJson, req, sizeof(req));
  printStatus("rpcBuildSubmitMultisigned", st);
  if (st != xahau::Status::Ok) {
    Serial.println(F("tx_json must start with '{' and pass control-char checks."));
    return;
  }
  Serial.println(F("Built request (not submitting placeholder to public net):"));
  printJsonSnippet(req);
  Serial.println(F("Fill real multi-signed fields, then client.rawRequest(req, ...)."));
""",
)

# ---- Objects (Vault/Loan) ----
add(
    "Objects",
    "FetchVaultIds",
    "account_objects (vault)",
    "account_objects type=vault",
    "XahauRpcClient::fetchVaultIds()  |  rpcParseVaultIds",
    """  char acct[48];
  if (!resolveAccount(acct, sizeof(acct), nullptr)) return;

  uint8_t ids[8][32];
  uint8_t n = 0;
  xahau::Status st = client.fetchVaultIds(acct, ids, 8, n);
  printStatus("fetchVaultIds", st);
  Serial.print(F("vault ids found: "));
  Serial.println(n);
  for (uint8_t i = 0; i < n; ++i) {
    char hex[65];
    xahau::bytesToHex(ids[i], 32, hex, sizeof(hex));
    Serial.print(F("  "));
    Serial.println(hex);
  }
  Serial.println(F("Tip: Vault objects are most common on Devnet (XAHAU_NETWORK=2)."));
""",
)

add(
    "Objects",
    "FetchLoanBrokerIds",
    "account_objects (loan broker)",
    "account_objects (LoanBroker)",
    "XahauRpcClient::fetchLoanBrokerIds()",
    """  char acct[48];
  if (!resolveAccount(acct, sizeof(acct), nullptr)) return;

  uint8_t ids[8][32];
  uint8_t n = 0;
  xahau::Status st = client.fetchLoanBrokerIds(acct, ids, 8, n);
  printStatus("fetchLoanBrokerIds", st);
  Serial.print(F("loan broker ids: "));
  Serial.println(n);
  for (uint8_t i = 0; i < n; ++i) {
    char hex[65];
    xahau::bytesToHex(ids[i], 32, hex, sizeof(hex));
    Serial.println(hex);
  }
""",
)

add(
    "Objects",
    "FetchLoanIds",
    "account_objects (loan)",
    "account_objects (Loan)",
    "XahauRpcClient::fetchLoanIds()",
    """  char acct[48];
  if (!resolveAccount(acct, sizeof(acct), nullptr)) return;

  uint8_t ids[8][32];
  uint8_t n = 0;
  xahau::Status st = client.fetchLoanIds(acct, ids, 8, n);
  printStatus("fetchLoanIds", st);
  Serial.print(F("loan ids: "));
  Serial.println(n);
  for (uint8_t i = 0; i < n; ++i) {
    char hex[65];
    xahau::bytesToHex(ids[i], 32, hex, sizeof(hex));
    Serial.println(hex);
  }
""",
)

# ---- Transport ----
add(
    "Transport",
    "RawRequest",
    "raw JSON-RPC over WSS",
    "any command via rawRequest",
    "XahauRpcClient::rawRequest + rpcBuild* builders",
    """  // Build any allowlisted request, or hand-craft JSON carefully.
  char req[128];
  xahau::Status st = xahau::rpcBuildPing(req, sizeof(req));
  printStatus("rpcBuildPing", st);

  st = client.rawRequest(req, s_respBig, sizeof(s_respBig));
  printStatus("rawRequest", st);
  printJsonSnippet(s_respBig);

  Serial.println(F("Pattern: rpcBuildX(...) -> client.rawRequest(req, resp, cap)"));
""",
)

add(
    "Transport",
    "EndToEndPrepareSubmit",
    "prepare + sign Payment + submit",
    "account_info + fee + submit (e2e)",
    "fetchSequenceAndLls + buildAndSignPayment + submitBlob",
    """  xahau::KeyPair keys;
  if (xahau::deriveKeysFromSeed(XAHAU_SEED, keys) != xahau::Status::Ok) {
    Serial.println(F("Set a funded XAHAU_SEED for this e2e demo."));
    return;
  }
  Serial.print(F("Address: "));
  Serial.println(keys.address);

  xahau::PaymentParams pay;
  // Send 1 drop to self (harmless smoke); change destination as needed.
  strncpy(pay.destination, keys.address, sizeof(pay.destination) - 1);
  pay.amount.kind = xahau::AmountKind::XrpDrops;
  pay.amount.drops = 1;

  xahau::Status st = xahau::sketch::prepareCommon(client, keys, pay.common);
  printStatus("prepareCommon", st);
  if (st != xahau::Status::Ok) {
    Serial.println(F("Fund the account on this network first."));
    xahau::secureClear(keys);
    return;
  }
  Serial.print(F("seq="));
  Serial.print(pay.common.sequence);
  Serial.print(F(" lls="));
  Serial.print(pay.common.lastLedgerSequence);
  Serial.print(F(" fee="));
  Serial.println((unsigned long)pay.common.feeDrops);

  char blob[2048];
  st = xahau::buildAndSignPayment(keys, pay, blob, sizeof(blob));
  printStatus("buildAndSignPayment", st);
  if (st != xahau::Status::Ok) {
    xahau::secureClear(keys);
    return;
  }

  xahau::SubmitResult sr;
  st = client.submitBlob(blob, sr);
  printStatus("submitBlob", st);
  Serial.print(F("engine_result: "));
  Serial.println(sr.engineResult);
  Serial.print(F("hash: "));
  Serial.println(sr.txHash);
  xahau::secureClear(keys);
""",
)

add(
    "Transport",
    "NetworkPresets",
    "network presets + WSS endpoint",
    "WsXahauTransport(NetworkEndpoint)",
    "kNetworkMainnet/Testnet/Devnet + networkEndpointValid",
    """  // Demonstrate switching public endpoints (no account required).
  const xahau::NetworkEndpoint* nets[3] = {
      &xahau::kNetworkMainnet, &xahau::kNetworkTestnet, &xahau::kNetworkDevnet};
  for (int i = 0; i < 3; ++i) {
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
""",
)


def write_all():
    counts = {}
    for ex in EXAMPLES:
        g = ex["group"]
        counts[g] = counts.get(g, 0) + 1
        d = RPC / g / ex["name"]
        d.mkdir(parents=True, exist_ok=True)
        body = sketch(ex["title"], ex["api"], g, ex["lib"], ex["body"])
        (d / f"{ex['name']}.ino").write_text(body, encoding="utf-8")

    # Group READMEs
    group_blurb = {
        "Server": "Server info methods: ping, fee, server_info/state, ledger*.",
        "Account": "Account methods: account_info, lines, tx history, objects, sequence helpers.",
        "Transaction": "Transaction methods: submit, fail_hard, tx lookup, multisign envelope.",
        "Objects": "Vault / Loan ID discovery via account_objects parsers.",
        "Transport": "WSS transport, raw requests, e2e prepare+sign+submit, network presets.",
    }
    for g, blurb in group_blurb.items():
        lines = [f"# WSS - {g}", "", blurb, "", "Sketches:", ""]
        for ex in EXAMPLES:
            if ex["group"] == g:
                lines.append(f"- [`{ex['name']}`](./{ex['name']}/{ex['name']}.ino) - `{ex['api']}`")
        lines.append("")
        (RPC / g / "README.md").write_text("\n".join(lines) + "\n", encoding="utf-8")

    # Root README
    lines = [
        "# XahauClient WSS examples (live WebSocket)",
        "",
        "Complete set of ESP32 sketches for every **wired JSON-RPC builder** and",
        "**XahauRpcClient** helper, over public WebSocket endpoints.",
        "",
        "Companion to transaction examples under [`../tx/`](../tx/).",
        "",
        "## Xahau public API map",
        "",
        "Aligned with [Public API Methods](https://Xahau.org/docs/references/http-websocket-apis/public-api-methods):",
        "",
        "| Group | Path | Sketches | Library coverage |",
        "|-------|------|---------:|------------------|",
        f"| Server | `wss/Server/` | {counts.get('Server', 0)} | ping, fee, server_info, server_state, ledger, ledger_closed |",
        f"| Account | `wss/Account/` | {counts.get('Account', 0)} | account_info, account_lines, account_tx, account_objects |",
        f"| Transaction | `wss/Transaction/` | {counts.get('Transaction', 0)} | submit, submit fail_hard, tx, submit_multisigned |",
        f"| Objects | `wss/Objects/` | {counts.get('Objects', 0)} | vault / loan broker / loan ID helpers |",
        f"| Transport | `wss/Transport/` | {counts.get('Transport', 0)} | rawRequest, e2e submit, network presets |",
        "",
        f"**Total: {len(EXAMPLES)} sketches**",
        "",
        "## How to use",
        "",
        "1. Open `examples/wss/<Group>/<Name>/`.",
        "2. Set `WIFI_SSID`, `WIFI_PASSWORD`, `XAHAU_NETWORK`.",
        "3. For account methods: set `ACCOUNT` **or** a funded `XAHAU_SEED`.",
        "4. Flash ESP32 (WebSockets library required).",
        "",
        "```cpp",
        'const char* WIFI_SSID    = "YOUR_WIFI_SSID";',
        'const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";',
        "const int   XAHAU_NETWORK = 1;  // testnet",
        'const char* ACCOUNT      = ""; // or r... ; empty uses seed address',
        'const char* XAHAU_SEED    = "sEd...";',
        "```",
        "",
        "## Flow",
        "",
        "```text",
        "WiFi -> WsXahauTransport(kNetwork*) -> XahauRpcClient",
        "     -> rpcBuild* / client.fetch* / client.submitBlob / rawRequest",
        "     -> Serial print",
        "```",
        "",
        "## Offline builders (no WiFi)",
        "",
        "Pure `rpcBuild*` / mock client sketches: [`../rpc/`](../rpc/).",
        "Host tests: `pio test -e native -f test_rpc`.",
        "",
        "## Related",
        "",
        "- Offline RPC: [`../rpc/`](../rpc/)",
        "- Tx examples: [`../tx/`](../tx/)",
        "- Smoke tests: [`../test/`](../test/)",
        "- Networks: [`../../docs/NETWORKS.md`](../../docs/NETWORKS.md)",
        "- Security / TLS pin: [`../../docs/SECURITY.md`](../../docs/SECURITY.md)",
        "",
        "## Regenerate",
        "",
        "```bash",
        "python protocol/scripts/generate_wss_examples.py",
        "```",
        "",
    ]
    (RPC / "README.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote {len(EXAMPLES)} WSS examples under {RPC}")
    for g, n in sorted(counts.items()):
        print(f"  {g}: {n}")


if __name__ == "__main__":
    write_all()

