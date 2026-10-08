// example_runtime.h - helpers for Arduino examples (optional)
// WiFi + seed + fetch sequence + submit over public network presets.
// Include after enabling XAHAU_FEATURE_NET and WiFi.
#pragma once

#include "../core/hex.h"
#include "../core/xahau_error.h"
#include "../core/xahau_types.h"
#include "../tx/tx_type.h"
#include "../crypto/keys.h"
#include "../net/networks.h"
#include "../rpc/client.h"
#include "../sign/sign.h"

#include <string.h>

#if defined(ARDUINO) || defined(XAHAU_PLATFORM_ARDUINO)
#include <WiFi.h>
#if defined(ESP32)
#include <Preferences.h>
#endif
#if XAHAU_FEATURE_NET
#include "../net/ws_transport.h"
#endif
#endif

namespace xahau {
namespace sketch {

/** Apply secp tfFullyCanonicalSig when needed (safe for Ed25519 too if 0). */
inline void applyCanonicalFlag(TxCommon& c, Curve curve) {
  if (curve == Curve::Secp256k1) {
    c.flags |= 0x80000000u;
  }
}

/**
 * Anti-double-submit for ClientTest harness: each flash injects a unique
 * XAHAU_HARNESS_RUN_ID. After a successful (or queued) submit we remember it in
 * NVS. If the board reboots (post-upload race + monitor DTR) and setup runs
 * again with the same id, we replay Serial markers and skip a second submit.
 * That prevents Success + tecNO_ENTRY pairs on Delete/Preauth/etc.
 */
#if defined(ESP32)
inline bool harnessReplayIfDone(const char* runId) {
  if (!runId || !runId[0]) return false;
  Preferences pref;
  if (!pref.begin("xahauh", true)) return false;
  const String rid = pref.getString("rid", "");
  if (rid != runId) {
    pref.end();
    return false;
  }
  const String hash = pref.getString("hash", "");
  const String eng = pref.getString("eng", "");
  const String addr = pref.getString("addr", "");
  const uint32_t seq = pref.getUInt("seq", 0);
  const bool tes = pref.getBool("tes", false);
  pref.end();

  Serial.println(F("=== XahauClient Â | harness_replay ==="));
  Serial.print(F("Library "));
  Serial.println(XAHAU_CLIENT_VERSION);
  Serial.print(F("Address: "));
  Serial.println(addr);
  Serial.print(F("Sequence: "));
  Serial.println(seq);
  Serial.println(F("[Xahau] submit status: Ok"));
  Serial.print(F("[Xahau] engine_result: "));
  Serial.println(eng);
  Serial.print(F("[Xahau] hash: "));
  Serial.println(hash);
  if (tes) {
    Serial.println(F("[Xahau] tesSUCCESS"));
  } else {
    Serial.println(F("[Xahau] not tesSUCCESS (see code)"));
  }
  Serial.println(F("[Xahau] harness_replay skipped duplicate submit"));
  Serial.println(F("Done."));
  return true;
}

inline void harnessRememberSubmit(const char* runId, const char* addr, uint32_t seq,
                                  const SubmitResult& sr, bool remember) {
  if (!remember || !runId || !runId[0] || !sr.txHash[0]) return;
  Preferences pref;
  if (!pref.begin("xahauh", false)) return;
  pref.putString("rid", runId);
  pref.putString("hash", sr.txHash);
  pref.putString("eng", sr.engineResult);
  pref.putString("addr", addr ? addr : "");
  pref.putUInt("seq", seq);
  pref.putBool("tes", sr.tesSuccess);
  pref.end();
}
#else
inline bool harnessReplayIfDone(const char* /*runId*/) { return false; }
inline void harnessRememberSubmit(const char* /*runId*/, const char* /*addr*/, uint32_t /*seq*/,
                                  const SubmitResult& /*sr*/, bool /*remember*/) {}
#endif

/**
 * Decode a 32-byte hash from hex (64 hex chars). Returns false on bad length.
 * Use for CheckID, Channel, URITokenID, etc.
 */
inline bool parseHash256(const char* hex, uint8_t out[32]) {
  if (!hex || !out) return false;
  size_t n = 0;
  if (hexToBytes(hex, out, 32, n) != Status::Ok || n != 32) return false;
  return true;
}

/** Decode 24-byte MPT issuance id from hex (48 hex chars). */
inline bool parseIssuanceId(const char* hex, uint8_t out[24]) {
  if (!hex || !out) return false;
  size_t n = 0;
  if (hexToBytes(hex, out, 24, n) != Status::Ok || n != 24) return false;
  return true;
}

#if defined(ARDUINO) || defined(XAHAU_PLATFORM_ARDUINO)

inline bool wifiConnect(const char* ssid, const char* password, uint32_t timeoutMs = 30000) {
  if (!ssid || !ssid[0] || strcmp(ssid, "YOUR_WIFI_SSID") == 0) {
    Serial.println(F("[Xahau] Set WIFI_SSID / WIFI_PASSWORD in the CONFIG block."));
    return false;
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  uint32_t start = millis();
  Serial.print(F("[Xahau] WiFi connecting"));
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > timeoutMs) {
      Serial.println(F("\n[Xahau] WiFi timeout."));
      return false;
    }
    delay(400);
    Serial.print('.');
  }
  Serial.println();
  Serial.print(F("[Xahau] WiFi OK  IP="));
  Serial.println(WiFi.localIP());
  return true;
}

/**
 * Pick public preset: 0=mainnet, 1=testnet (default).
 * Index 2 maps to testnet (no separate public Xahau "devnet" preset).
 */
inline const NetworkEndpoint& networkFromIndex(int idx) {
  if (idx == 0) return kNetworkMainnet;
  return kNetworkTestnet;
}

#if XAHAU_FEATURE_NET

/**
 * TLS policy by network.
 *
 * Mainnet: fail-closed - requireSslPin=true, insecureSsl=false.
 *   You MUST call setSslFingerprint(...) or setSslCaCert(...) before request(),
 *   or beginTlsSession returns InvalidArgument.
 *
 * Testnet/Custom: allow unpinned SSL for public-cluster convenience
 *   (still encrypted; not MITM-proof). Override with setRequireSslPin(true) anytime.
 *
 * Call after setEndpoint / construction, before prepareCommon/submit.
 */
inline void applyDefaultTlsPolicy(WsXahauTransport& ws) {
  if (ws.networkId() == NetworkId::Mainnet) {
    ws.setRequireSslPin(true);
    ws.setInsecureSsl(false);
    ws.setKeepAlive(true);
  } else {
    ws.setRequireSslPin(false);
    ws.setInsecureSsl(true);
    ws.setKeepAlive(true);
  }
}

/**
 * Apply endpoint + default TLS policy. Prefer this in production sketches:
 *   applyNetworkTransport(ws, networkFromIndex(XAHAU_NETWORK));
 * For Mainnet, set fingerprint/CA after this call.
 */
inline void applyNetworkTransport(WsXahauTransport& ws, const NetworkEndpoint& ep) {
  ws.setEndpoint(ep);
  applyDefaultTlsPolicy(ws);
}

/** @deprecated Prefer txMinFeeDrops(TxType) - same value as AMMCreate/VaultCreate/AccountDelete. */
constexpr uint64_t kSpecialTxFeeMinDrops = 200000ULL;

/** Raise fee to at least minDrops (no-op if already higher). */
inline void ensureFeeAtLeast(TxCommon& common, uint64_t minDrops) {
  if (minDrops > 0 && common.feeDrops < minDrops) {
    common.feeDrops = minDrops;
  }
}

/**
 * Fetch Sequence + LastLedgerSequence + open fee for this account.
 * Fee falls back to feeFallbackDrops if server fee is 0.
 *
 * Retries account_info / fee on Timeout / Rejected / DecodeFailed - long
 * sequence runs against public Testnet often hit transient WSS flakes after
 * multi-minute waits (e.g. escrow), which previously cascaded as mass skips.
 *
 * Prefer prepareCommonForTx() for special-cost types (AccountDelete,
 * AccountDelete). bagSetCommon also enforces txMinFeeDrops(type) at encode time.
 */
// Apply on-wire NetworkID from a public preset (required for every Xahau tx).
// Call before fill/sign when you know the target network.
// NOTE: do not put "*/" sequences inside block comments in this header.
inline void applyNetworkId(TxCommon& common, const NetworkEndpoint& ep) {
  common.networkId = ep.protocolNetworkId;
}

inline void applyNetworkId(TxCommon& common, NetworkId id) {
  common.networkId = protocolNetworkIdFor(id);
}

inline Status prepareCommon(XahauRpcClient& client, const KeyPair& keys, TxCommon& common,
                            uint64_t feeFallbackDrops = 12, uint64_t feeMinDrops = 0) {
  // Callers should set common.networkId via applyNetworkId() for the target chain.
  // If still unset, default to testnet so offline experiments still encode a valid Xahau field.
  if (common.networkId < 1025u) {
    common.networkId = kProtocolNetworkIdTestnet;
  }
  uint32_t seq = 0, lls = 0;
  Status st = Status::Timeout;
  for (int attempt = 0; attempt < 4; ++attempt) {
    if (attempt > 0) {
#if defined(ARDUINO) || defined(XAHAU_PLATFORM_ARDUINO)
      delay(800 * attempt);
      Serial.print(F("[Xahau] prepareCommon retry "));
      Serial.println(attempt);
#endif
    }
    st = client.fetchSequenceAndLls(keys.address, seq, lls);
    if (st == Status::Ok) break;
    // Soft-fail only on transient transport/parse issues; keep hard errors.
    if (st != Status::Timeout && st != Status::Rejected && st != Status::DecodeFailed) {
      return st;
    }
  }
  if (st != Status::Ok) return st;

  common.sequence = seq;
  common.lastLedgerSequence = lls;
  const uint64_t priorFee = common.feeDrops;
  uint64_t fee = 0;
  st = Status::Timeout;
  for (int attempt = 0; attempt < 3; ++attempt) {
    if (attempt > 0) {
#if defined(ARDUINO) || defined(XAHAU_PLATFORM_ARDUINO)
      delay(500 * attempt);
#endif
    }
    st = client.fetchFeeDrops(fee);
    if (st == Status::Ok && fee > 0) break;
  }
  if (st == Status::Ok && fee > 0) {
    common.feeDrops = fee;
  } else if (common.feeDrops == 0) {
    common.feeDrops = feeFallbackDrops;
  }
  ensureFeeAtLeast(common, priorFee);
  ensureFeeAtLeast(common, feeMinDrops);
  applyCanonicalFlag(common, keys.curve);
  return Status::Ok;
}

/**
 * Like prepareCommon, but applies the library special-fee table for `txType`
 * (e.g. TxType::AccountDelete -> 200000 drops). Use this in all live sketches.
 *
 * SetHook: txMinFeeDrops is a soft floor only. Pass createCodeLen * 500 via
 * extraMinDrops (setHookCreateFeeDrops) so novel CreateCode installs are not
 * underfunded. Preferred exact path: unsigned tx_blob into fee RPC
 * (rpcBuildFeeWithTxBlob) then ensureFeeAtLeast(base_fee) before re-sign.
 */
inline Status prepareCommonForTx(XahauRpcClient& client, const KeyPair& keys, TxCommon& common,
                                 TxType txType, uint64_t feeFallbackDrops = 12,
                                 uint64_t extraMinDrops = 0) {
  uint64_t floor = txMinFeeDrops(txType);
  if (extraMinDrops > floor) floor = extraMinDrops;
  Status st = prepareCommon(client, keys, common, feeFallbackDrops, floor);
  if (st != Status::Ok) return st;
  ensureFeeAtLeast(common, floor);
  return Status::Ok;
}
/**
 * Submit signed hex blob; prints engine_result to Serial.
 *
 * Returns Status::Ok (tes), Status::Queued (terQUEUED), or Status::Rejected.
 * On Queued/Timeout: do NOT re-sign with a new Sequence - call waitForOutcome /
 * fetchTxResult with the returned hash first.
 *
 * @param harnessRunId  Optional ClientTest flash id; when set, remembers
 *   tesSUCCESS / terQUEUED so a second boot of the same flash will not submit again.
 * @param accountAddr   Optional classic address for replay Serial output.
 * @param sequence      Optional Sequence used in the signed tx (for replay).
 */
inline Status submitAndLog(XahauRpcClient& client, const char* blobHex,
                           const char* harnessRunId = nullptr, const char* accountAddr = nullptr,
                           uint32_t sequence = 0) {
  SubmitResult sr;
  Status st = client.submitBlob(blobHex, sr);
  Serial.print(F("[Xahau] submit status: "));
  Serial.println(statusName(st));
  Serial.print(F("[Xahau] engine_result: "));
  Serial.println(sr.engineResult);
  Serial.print(F("[Xahau] hash: "));
  Serial.println(sr.txHash);
  // Status::Queued = terQUEUED (provisional). Do not re-sign; resolve via hash.
  const bool queued = sr.queued || st == Status::Queued || rpcIsTerQueued(sr.engineResult);
  if (sr.tesSuccess) {
    Serial.println(F("[Xahau] tesSUCCESS"));
  } else if (queued) {
    Serial.println(F("[Xahau] terQUEUED (provisional - wait for final via hash)"));
  } else {
    Serial.println(F("[Xahau] not tesSUCCESS (see code)"));
  }
  // Remember applied / queued only - never tefPAST_SEQ (allows one re-flash).
  const bool remember = sr.tesSuccess || queued;
  harnessRememberSubmit(harnessRunId, accountAddr, sequence, sr, remember);
  return st;
}

/**
 * Submit then wait for a final ledger result when the submit is provisional
 * (Queued) or the transport timed out after a hash was returned.
 *
 * Never re-signs. Safe default for production sketches on public clusters.
 *
 * @param lastLedgerSequence  from prepareCommon (p.common.lastLedgerSequence)
 * @param waitTimeoutMs       poll budget (default 60s)
 */
inline Status submitAndWait(XahauRpcClient& client, const char* blobHex, uint32_t lastLedgerSequence,
                            SubmitResult& out, uint32_t waitTimeoutMs = 60000,
                            const char* harnessRunId = nullptr, const char* accountAddr = nullptr,
                            uint32_t sequence = 0,
                            const char* localHashFallback = nullptr) {
  Status st = client.submitBlob(blobHex, out);
  Serial.print(F("[Xahau] submit status: "));
  Serial.println(statusName(st));
  Serial.print(F("[Xahau] engine_result: "));
  Serial.println(out.engineResult);
  Serial.print(F("[Xahau] hash: "));
  Serial.println(out.txHash);

  if (st == Status::Ok && out.tesSuccess) {
    Serial.println(F("[Xahau] tesSUCCESS"));
    harnessRememberSubmit(harnessRunId, accountAddr, sequence, out, true);
    return st;
  }

  // Provisional or ambiguous: resolve by hash - never re-sign.
  // Prefer server hash; if submit meta timed out empty, use caller-computed HASH_TX_ID.
  const char* pollHash = out.txHash[0] ? out.txHash : localHashFallback;
  if ((st == Status::Queued || st == Status::Timeout || out.queued) && pollHash && pollHash[0]) {
    Serial.println(F("[Xahau] waiting for final outcome..."));
    if (!out.txHash[0] && localHashFallback && localHashFallback[0]) {
      Serial.println(F("[Xahau] submit meta missing; using local_tx_hash"));
    }
    st = client.waitForOutcome(pollHash, lastLedgerSequence, waitTimeoutMs, out, 500);
    Serial.print(F("[Xahau] final status: "));
    Serial.println(statusName(st));
    Serial.print(F("[Xahau] engine_result: "));
    Serial.println(out.engineResult);
  }

  if (out.tesSuccess || st == Status::Ok) {
    Serial.println(F("[Xahau] tesSUCCESS"));
    harnessRememberSubmit(harnessRunId, accountAddr, sequence, out, true);
    return Status::Ok;
  }
  if (st == Status::Queued || out.queued) {
    Serial.println(F("[Xahau] terQUEUED (still provisional after wait)"));
    harnessRememberSubmit(harnessRunId, accountAddr, sequence, out, true);
    return Status::Queued;
  }
  Serial.println(F("[Xahau] not tesSUCCESS (see code)"));
  return st;
}

#endif  // XAHAU_FEATURE_NET
#endif  // ARDUINO

}  // namespace sketch
}  // namespace xahau
