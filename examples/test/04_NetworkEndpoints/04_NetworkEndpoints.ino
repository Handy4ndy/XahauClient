/**
 * XahauClient Example - 04_NetworkEndpoints
 *
 * Network endpoint presets
 *
 * Demonstrates: Prints Mainnet/Testnet host and NetworkID presets.
 * Hardware: ESP32 / ESP32-S3 (offline; no Wi-Fi required)
 * Network: N/A (offline). Use Testnet seeds only if you adapt this sketch for live use.
 * Config: Replace any seed placeholders (e.g. sEdPLACEHOLDER_REPLACE_ME) before adapting for live use. Never commit real secrets.
 *
 * SPDX-License-Identifier: MIT
 */
#include <XahauClient.h>

static void printEp(const char* label, const xahau::NetworkEndpoint& ep) {
  Serial.println(label);
  Serial.print(F("  name:  "));
  Serial.println(ep.name);
  Serial.print(F("  WSS:   "));
  Serial.print(ep.wsHost);
  Serial.print(F(":"));
  Serial.print(ep.wsPort);
  Serial.println(ep.wsPath);
  Serial.print(F("  HTTPS: "));
  Serial.print(ep.rpcHost);
  Serial.print(F(":"));
  Serial.println(ep.rpcPort);
  Serial.print(F("  NetworkID: "));
  Serial.println(ep.protocolNetworkId);
  if (ep.faucetHint) {
    Serial.print(F("  faucet: "));
    Serial.println(ep.faucetHint);
  }
  Serial.print(F("  valid: "));
  Serial.println(xahau::networkEndpointValid(ep) ? F("yes") : F("NO"));
  Serial.println(ep.notes);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("=== XahauClient network matrix ==="));
  Serial.print(F("allPublicNetworksWired: "));
  Serial.println(xahau::allPublicNetworksWired() ? F("YES") : F("NO"));

  printEp("MAINNET", xahau::kNetworkMainnet);
  printEp("TESTNET", xahau::kNetworkTestnet);

  Serial.println(F("Live-smoke recommendations (prefer Testnet):"));
  Serial.print(F("  Payment      -> "));
  Serial.println(xahau::networkPreset(xahau::recommendedLiveNetwork(TxType::Payment)).name);
  Serial.print(F("  URITokenMint -> "));
  Serial.println(xahau::networkPreset(xahau::recommendedLiveNetwork(TxType::URITokenMint)).name);
  Serial.print(F("  Remit        -> "));
  Serial.println(xahau::networkPreset(xahau::recommendedLiveNetwork(TxType::Remit)).name);
  Serial.print(F("  NFTokenModify live? "));
  Serial.println(xahau::txLiveSupport(TxType::NFTokenModify) == xahau::TxLiveSupport::EncodeOnly
                     ? F("EncodeOnly (prefer URIToken)")
                     : F("see txLiveSupport"));
  Serial.print(F("  VaultCreate live? "));
  Serial.println(xahau::isTxLiveEnabledOn(TxType::VaultCreate, xahau::NetworkId::Testnet)
                     ? F("yes")
                     : F("no (encode-only stub)"));

#if XAHAU_FEATURE_NET
  Serial.println(F("NET=1: WsXahauTransport ws(kNetworkTestnet);"));
#else
  Serial.println(F("NET=0: encode offline; enable XAHAU_FEATURE_NET for device WS."));
#endif
}

void loop() {}
