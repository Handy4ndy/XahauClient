/**
 * XahauClient Example — SetHook
 *
 * SetHook transaction
 *
 * Demonstrates: Builds and optionally submits SetHook (Hooks CreateCode).
 * Hardware: ESP32 / ESP32-S3 (Wi-Fi + WebSocketsClient when XAHAU_FEATURE_NET=1)
 * Network: Xahau Testnet by default (NetworkID 21338); set XAHAU_NETWORK=0 for Mainnet (21337).
 * Config: Set WIFI_SSID / WIFI_PASSWORD and XAHAU_SEED placeholders (sEdPLACEHOLDER_REPLACE_ME). Prefer DO_SUBMIT=false until ready. Never commit real credentials.
 *
 * SPDX-License-Identifier: MIT
 */
#define XAHAU_FEATURE_NET 1
#include <WiFi.h>
#include <XahauClient.h>
#include <xahau/sketch/example_runtime.h>
#include <string.h>
#if defined(ESP_PLATFORM) && defined(SET_LOOP_TASK_STACK_SIZE)
SET_LOOP_TASK_STACK_SIZE(24 * 1024);
#endif

const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* XAHAU_SEED    = "sEdPLACEHOLDER_REPLACE_ME";
// 0 = Mainnet (NetworkID 21337), 1 = Testnet (NetworkID 21338)
const int   XAHAU_NETWORK = 1;
const bool  DO_SUBMIT     = false;  // offline by default for library example
const bool  PRINT_BLOB    = true;

// Exact CreateCode from starter(16) / task (no spaces)
const char* CREATE_CODE_HEX =
  "0061736D0100000001420A60057F7F7F7F7F017E60027F7F017E60037F7F7F017E6000017E60037F7F7E017E60047F7F7F7F017E60027F7E017E60037E7F7F017E60027F7F017F60017F017E02C3010D03656E76057472616365000003656E760C686F6F6B5F6163636F756E74000103656E760A6F74786E5F6669656C64000203656E76096F74786E5F74797065000303656E7608726F6C6C6261636B000403656E760A6F74786E5F706172616D000503656E760973746174655F736574000503656E7606616363657074000403656E76057374617465000503656E7609666C6F61745F736574000603656E7609666C6F61745F696E74000703656E760974726163655F6E756D000403656E76025F670008030201090503010002062B077F0141F08C040B7F004180080B7F0041E50C0B7F004180080B7F0041F08C040B7F0041000B7F0041010B07080104686F6F6B000D0AB7850001B3850002087E017F230041E0006B2200240041B70C412A418E0C4129410010001A200041406B411410011A200041206A41144181802010021A1003220250450440200242E3005104400240200029034020002903205104402000290348200029032851044020002802502000280230460D020B0B41BF0A412C423310041A0B2000410E6A41E40C2D00003A0000200041E20C2F00003B010C200041186A41082000410C6A41031005420852044041B20B4132423B10041A0B200041186A41082000410C6A41031006420053044041B3094133423F10041A0B41930A412C42C10010071A0B418709412C42C50010071A0B200041166A41E40C2D00003A0000200041E20C2F00003B0114200041186A4108200041146A41031008420852044041E609412D42CD0010041A0B200031001F2102200031001E2101200031001D2103200031001C2104200031001B2105200031001A2106200031001920003100182108419F084108200041406B4114410110001A4196084108200041206A4114410110001A42308620084238868420064228868420054220868420044218868420034210868420014208868420002903402000290320510440024020002903482000290328520D0020002802502000280230470D0041E208412542D60010071A0B0B20027C21012000410C6A4108418180181002417A20002C000C2209410048047E427E052000310013200031000D4230862009AD42118342388684200031000E42288684200031000F422086842000310010421886842000310011421086842000310012420886847C0B2202100941004101100A2104418008410A2001100B1A418B08410A2004100B1A420852044041E40B412A42E40010071A0B2001BA2002B9440000000080842E41A364044041A808413A42EB0010041A0B41EB0A41C70042EE0010071A41014101100C1A200041E0006A240042000B0BED0401004180080BE5046D696E5F616D6F756E7400616D6F756E745F696E74006F74786E5F61636300686F6F6B5F61636300424950203A3A204572726F72203A3A20496E636F6D696E67207061796D656E742069732062656C6F77206D696E696D756D20616D6F756E742E00424950203A3A204163636570746564203A3A204F7574676F696E67207061796D656E742E00424950203A3A204163636570746564203A3A204E6F6E2D5061796D656E74207472616E73616374696F6E2E00424950203A3A204572726F72203A3A20436F756C64206E6F7420736574206D696E20616D6F756E7420696E2073746174652E00424950203A3A204572726F72203A3A204D696E20616D6F756E74206E6F742073657420696E2073746174652E00424950203A3A204163636570746564203A3A204D696E20616D6F756E742073657420696E2073746174652E00424950203A3A204572726F72203A3A204F6E6C7920686F6F6B206F776E65722063616E20696E766F6B652E00424950203A3A204163636570746564203A3A20496E636F6D696E6720616D6F756E742061626F766520746865206D696E696D756D20616D6F756E74207468726573686F6C642E00424950203A3A204572726F72203A3A204D494E20706172616D65746572206E6F7420736574206F7220696E76616C69642E00424950203A3A204163636570746564203A3A20496E636F6D696E6720494F552041636365707465642E00424950203A3A20426173696320496E766F6B6520506172616D65746572203A3A2043616C6C65642E0022424950203A3A20426173696320496E766F6B6520506172616D65746572203A3A2043616C6C65642E22004D494E";

// HookOn: fire on ttPAYMENT (0) + ttINVOKE (99); bit22 clear (no SetHook)
const char* HOOK_ON_HEX =
  "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF7FFFFFFFFFFFFFFFFFFBFFFFE";
// HookNamespace = SHA256("BIP")
const char* HOOK_NS_HEX =
  "CE7A503556DAD15D08741BF67FF2E4CC6869BFE3E3ACA4A9CB35EF2EAFBCD330";

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println();
  Serial.println(F("=== XahauClient - SetHook ==="));
  Serial.print(F("Library "));
  Serial.println(XAHAU_CLIENT_VERSION);

  xahau::KeyPair keys;
  xahau::Status st = xahau::deriveKeysFromSeed(XAHAU_SEED, keys);
  if (st != xahau::Status::Ok) {
    Serial.print(F("deriveKeysFromSeed failed: "));
    Serial.println(xahau::statusName(st));
    return;
  }
  Serial.print(F("Address: "));
  Serial.println(keys.address);

  const xahau::NetworkEndpoint& net = xahau::sketch::networkFromIndex(XAHAU_NETWORK);
  Serial.print(F("Network: "));
  Serial.println(net.name);

  static uint8_t s_code[8192];  // Tier A CreateCode cap
  size_t codeLen = 0;
  st = xahau::hexToBytes(CREATE_CODE_HEX, s_code, sizeof(s_code), codeLen);
  if (st != xahau::Status::Ok || codeLen == 0) {
    Serial.println(F("CreateCode hex decode failed"));
    xahau::secureClear(keys);
    return;
  }
  Serial.print(F("CreateCode bytes: "));
  Serial.println((unsigned)codeLen);

  xahau::SetHookParams p;
  p.createCode = s_code;
  p.createCodeLen = static_cast<uint16_t>(codeLen);
  p.hookApiVersion = 0;
  p.hookFlags = 1;  // hsfOVERRIDE
  if (!xahau::sketch::parseHash256(HOOK_ON_HEX, p.hookOn)) {
    Serial.println(F("Bad HookOn"));
    xahau::secureClear(keys);
    return;
  }
  if (!xahau::sketch::parseHash256(HOOK_NS_HEX, p.hookNamespace)) {
    Serial.println(F("Bad HookNamespace"));
    xahau::secureClear(keys);
    return;
  }
  // Novel CreateCode: 500 drops/byte (prefer fee+tx_blob for exact); not hard flat 2M
  p.common.feeDrops = xahau::setHookCreateFeeDrops(p.createCodeLen);
  p.common.sequence = 1;
  p.common.lastLedgerSequence = 100;
  xahau::sketch::applyNetworkId(p.common, net);
  xahau::sketch::applyCanonicalFlag(p.common, keys.curve);

  static char s_blob[17408];  // Tier A signed hex + submit margin
  static xahau::TxFieldBag s_bag;
  auto doSign = [&]() -> xahau::Status {
    s_bag.clear();
    xahau::Status s = xahau::fillSetHook(p, keys.accountId, keys.publicKey, s_bag);
    if (s != xahau::Status::Ok) return s;
    return xahau::signTx(s_bag, keys, s_blob, sizeof(s_blob));
  };

  st = doSign();
  Serial.print(F("sign: "));
  Serial.println(xahau::statusName(st));
  if (st != xahau::Status::Ok) {
    xahau::secureClear(keys);
    return;
  }
  if (PRINT_BLOB) {
    Serial.println(F("Signed blob (hex):"));
    Serial.println(s_blob);
  }

  if (!DO_SUBMIT) {
    Serial.println(F("DO_SUBMIT=false - offline only. Done."));
    xahau::secureClear(keys);
    return;
  }

#if XAHAU_FEATURE_NET
  if (!xahau::sketch::wifiConnect(WIFI_SSID, WIFI_PASSWORD)) {
    xahau::secureClear(keys);
    return;
  }

  static xahau::WsXahauTransport s_ws;
  static xahau::XahauRpcClient s_client(s_ws);
  s_client.setTimeoutMs(90000);  // SetHook WASM submit/meta can be slow
  xahau::sketch::applyNetworkTransport(s_ws, net);
  s_ws.setRequireSslPin(false);
  s_ws.setInsecureSsl(true);

  const uint64_t hookFee = xahau::setHookCreateFeeDrops(p.createCodeLen);
  st = xahau::sketch::prepareCommonForTx(s_client, keys, p.common, xahau::TxType::SetHook,
                                         hookFee, hookFee);
  if (st != xahau::Status::Ok) {
    Serial.print(F("prepareCommonForTx failed: "));
    Serial.println(xahau::statusName(st));
    xahau::secureClear(keys);
    return;
  }
  xahau::sketch::applyNetworkId(p.common, net);
  xahau::sketch::ensureFeeAtLeast(p.common, hookFee);

  Serial.print(F("Sequence: "));
  Serial.println(p.common.sequence);
  Serial.print(F("LLS:      "));
  Serial.println(p.common.lastLedgerSequence);
  Serial.print(F("Fee:      "));
  Serial.println((unsigned long)p.common.feeDrops);
  Serial.print(F("NetworkID:"));
  Serial.println(p.common.networkId);

  st = doSign();
  if (st != xahau::Status::Ok) {
    Serial.print(F("re-sign failed: "));
    Serial.println(xahau::statusName(st));
    xahau::secureClear(keys);
    return;
  }
  xahau::sketch::submitAndLog(s_client, s_blob);
#else
  Serial.println(F("XAHAU_FEATURE_NET disabled"));
#endif

  xahau::secureClear(keys);
  Serial.println(F("Done."));
}

void loop() {}
