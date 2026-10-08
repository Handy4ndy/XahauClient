# XahauClient API reference

Umbrella include:

```cpp
#include <XahauClient.h>
```

Optional network (ESP32):

```cpp
#define XAHAU_FEATURE_NET 1
#include <XahauClient.h>
#include <xahau/net/ws_transport.h>
```

Version macros: `XAHAU_CLIENT_VERSION`, `XAHAU_CLIENT_VERSION_MAJOR/MINOR/PATCH`.

---

## Layers

```text
Sketch / application
  ├── XahauRpcClient + IXahauTransport   (Mock or Ws)
  ├── buildAndSign* / fill* + signTx
  │     └── TxFieldBag → serialize → STX sign → hex
  ├── deriveKeysFromSeed / secureClear
  └── NetworkEndpoint presets (mainnet / testnet; NetworkID 21337/21338)
```

---

## Keys (`xahau/crypto/keys.h`)

| API | Description |
|-----|-------------|
| `deriveKeysFromSeed(seed, KeyPair&)` | Auto-detect `sEd…` → Ed25519, else secp256k1 |
| `deriveKeysFromSeed(seed, Curve, KeyPair&)` | Explicit curve |
| `encodeEd25519Seed` / `encodeSecp256k1Seed` | 16-byte entropy → family seed (base58) |
| `seedFromEntropy(entropy16, Curve, out, cap)` | Encode seed only (no RNG inside library) |
| `generateKeyPairFromEntropy(...)` | Encode seed + derive `KeyPair` |
| `secureClear(KeyPair&)` | Wipe private material |
| `accountIdFromPublicKey` | Account ID from 33-byte pubkey |

C-friendly aliases: `xahauDeriveKeysFromSeed`, `xahauSeedFromEntropy`,
`xahauGenerateKeyPairFromEntropy`, `xahauSecureClear`, `XahauKeyPair`.

### Generating a new seed

The library **does not** collect hardware RNG. The application must fill 16 bytes
from a CSPRNG (e.g. ESP32 `esp_fill_random` only when RF/SAR entropy is active),
then call `seedFromEntropy` or `generateKeyPairFromEntropy`.

```cpp
uint8_t entropy[16];
// app: entropyFillRandom(entropy, 16);  // fail closed if RNG not ready
char seed[xahau::kFamilySeedBufSize];
XahauKeyPair keys;
xahauGenerateKeyPairFromEntropy(entropy, XahauCurve::Ed25519, seed, sizeof(seed), keys);
// ALWAYS show seed and require offline backup before vault encrypt.
// After encryption, the seed is NOT recoverable from the device.
secureZero(entropy, sizeof(entropy));
xahauSecureClear(keys);
```

Host goldens: `test/vectors/keys/generate_seed.json` (aligned with `ripple-keypairs`).

---

## Transaction builders (`xahau/tx/`)

### Pattern

1. Fill a `*Params` struct (`TxCommon`: sequence, LLS, fee, flags).
2. Call either:
   - `fillName(params, accountId, pubKey, bag)` then `signTx(bag, keys, hex, cap)`, or
   - `buildAndSignName(keys, params, hex, cap)` where available.

### Modules

| Header | Types |
|--------|--------|
| `payment.h` | Payment |
| `account_set.h` | AccountSet |
| `trust_set.h` | TrustSet |
| `offer.h` | OfferCreate, OfferCancel |
| `account_keys.h` | TicketCreate, SetRegularKey, SignerListSet |
| `phase2_extra.h` | Escrow*, Check*, PayChan*, DepositPreauth, AccountDelete, Clawback, OracleDelete |
| `phase4_extra.h` | `fillOracleSet` only (XRPL-port EncodeOnly families removed 2026-09-28; catalog codes remain in `tx_type.h`) |
| `xahau_extra.h` | URIToken*, Remit, ClaimReward, CronSet, SetRemarks, Invoke, Import, SetHook (`fillSetHook` + `signTx`; **no** `buildAndSignSetHook`), plus `buildAndSignOracleSet` |
| `tx_type.h` | Full `TxType` catalog + maturity |

`SetHook` is fill-then-`signTx` only (large CreateCode / Tier A caps). Prefer `examples/tx/Other/SetHook/` for the pattern.

### Catalog helpers

| API | Description |
|-----|-------------|
| `txTypeCatalog()` / `txTypeCatalogCount()` | Registry table |
| `txTypeName(TxType)` | String name |
| `txTypeLookupByName` | Name → info |

### Live-support helpers (`networks.h`)

| API | Description |
|-----|-------------|
| `txLiveSupport(TxType)` | AllPublic / EncodeOnly / NotUserSubmit |
| `recommendedLiveNetwork(TxType)` | Suggested public net for smoke tests (`Custom` if none live) |
| `amendmentStatus` / `amendmentEnabledOn` | Per-net amendment ON/OFF/ABSENT (2026-07-14 matrix) |
| `txPrimaryAmendment` | Tx type → gating amendment |
| `isTxLiveEnabledOn(TxType, NetworkId)` | Whether live submit is expected on that public net |

Offline encode/sign applies to **shipped** fill/sign helpers. Catalog-only `TxType` codes (former EncodeOnly XRPL-port families) have **no** fill/sign in `src` after 2026-09-28 — see [ENCODE_ONLY_STUBS.md](ENCODE_ONLY_STUBS.md). Use `isTxLiveEnabledOn` to skip live smoke when an amendment is absent.

---

## Codec (`xahau/codec/`)

| Type / API | Description |
|------------|-------------|
| `TxFieldBag` | Fixed slots + arena; sorted field codes |
| `bagSetCommon` | TransactionType, Account, SigningPubKey, Fee, Sequence, … |
| `xahauSerialize` | Canonical STObject bytes |
| `STNumber` helpers | Decimal → 12-byte STNumber |

Caps: `XAHAU_TX_FIELD_BAG_MAX_SLOTS` (default 32), `XAHAU_TX_FIELD_BAG_ARENA` (default 12288; Tier A SetHook CreateCode <= 8 KiB).

---

## Signing (`xahau/sign/`)

| API | Description |
|-----|-------------|
| `signTx(bag, keys, outHex, cap)` | Full STX sign; host stack / ESP static BSS (`XAHAU_SIGN_*_CAP` default 8704/8712; Tier A SetHook) |
| `signRaw` | Low-level sign of message bytes |

Secp256k1: RFC 6979 deterministic k + low-S + BIP66 DER.  
Ed25519: 64-byte raw signature.

---

## RPC builders (`xahau/rpc/rpc.h`)

Pure functions (no I/O). Strings are **validated / escaped** before JSON emission.

| Builder | Command |
|---------|---------|
| `rpcBuildAccountInfo` / `Ex` | `account_info` |
| `rpcBuildAccountLines` | `account_lines` |
| `rpcBuildAccountTx` | `account_tx` |
| `rpcBuildAccountObjects` | `account_objects` |
| `rpcBuildAccountChannels` | `account_channels` |
| `rpcBuildAccountCurrencies` | `account_currencies` |
| `rpcBuildAccountNfts` | `account_nfts` |
| `rpcBuildAccountOffers` | `account_offers` |
| `rpcBuildGatewayBalances` | `gateway_balances` |
| `rpcBuildNoRippleCheck` | `noripple_check` |
| `rpcBuildSubmit` / `Ex` | `submit` (+ optional `fail_hard`) |
| `rpcBuildSubmitMultisigned` | `submit_multisigned` |
| `rpcBuildTx` | `tx` |
| `rpcBuildTransactionEntry` | `transaction_entry` |
| `rpcBuildFee` / `rpcBuildFeeWithTxBlob` | `fee` (+ optional unsigned `tx_blob` for Hooks exact fee) |
| `rpcBuildFeature` | `feature` |
| `rpcBuildServerInfo` / `ServerState` / `ServerDefinitions` | server_* |
| `rpcBuildManifest` / `Random` / `Ping` | `manifest` / `random` / `ping` |
| `rpcBuildLedger` / `LedgerClosed` / `LedgerCurrent` / `LedgerData` | ledger* |
| `rpcBuildLedgerEntryByIndex` / `AccountRoot` | `ledger_entry` |
| `rpcBuildDepositAuthorized` / `BookOffers` | path/DEX checks |
| `rpcBuildNftBuyOffers` / `SellOffers` / `NftInfo` / `NftHistory` / `NftsByIssuer` | NFT / Clio |
| `rpcBuildChannelVerify` | `channel_verify` |
| `rpcBuildSubscribeStreams` / `Accounts` / `AccountsProposed` / combined | `subscribe` |
| `rpcBuildSubscribeEx` / `UnsubscribeEx` | `SubscribeParams` |
| `rpcBuildUnsubscribe*` / `rpcBuildSubscribeLedger` | `unsubscribe` |

**Wallet product plan:** builders land first → host + ClientTest RPC suite verify → then wallet-v2 wires `rawRequest` / thin helpers to showcase capacity. Deferred for later waves: `path_find`, `ripple_path_find`, full `ledger_entry` type matrix, `mpt_holders`, server-side `sign*`.

### Parsers (lightweight)

| API | Description |
|-----|-------------|
| `rpcParseAccountInfoSnapshot` | Sequence, balance, owner count, ledger |
| `rpcParseFeeDrops` | Base / open / median fee |
| `rpcParseSubmitEngineResult` | `engine_result` + hash |
| `rpcParseLedgerIndex` | Ledger index fields |
| `rpcParseObjectIds` / `rpcParseFirstObjectId` | Object indexes |
| `rpcIsTesSuccess` | `engine_result` starts with `tes` |
| `rpcParseStreamMessageType` | Classify push vs RPC response (all documented stream `type`s) |
| `rpcParseLedgerClosedEvent` | Index / hash / txn_count / fee_base / reserves / network_id |
| `rpcParseLedgerClosedValidatedLedgers` | Long `validated_ledgers` range into caller buffer |
| `rpcParseTransactionStreamEvent` | Hash / type / Account / Destination / engine_result / ledger_index |
| `rpcIsReportingUnsupported` | Clio error for unsupported streams |

---

## Client (`xahau/rpc/client.h`)

```cpp
XahauRpcClient client(transport);
client.setTimeoutMs(30000);          // default 30s (public clusters)
client.setLedgerOffset(40);          // default 40; values below 40 still floor to kMinLedgerOffset
client.setResponseBuffer(buf, cap);  // optional larger buffer
```

| Method | Description |
|--------|-------------|
| `ping` | Connectivity |
| `fetchAccountInfo` | Snapshot |
| `fetchSequenceAndLls` | Sequence + LLS (`ledger + effectiveLedgerOffset()`) |
| `fetchFeeDrops` / `fetchFeeAndLedger` | Fee (+ ledger / LLS) |
| `submitBlob` / `submitBlobFailHard` | Submit signed hex. Returns `Ok` (tes), **`Queued`** (terQUEUED), or `Rejected`. Same-blob retry once on transport `Timeout`. |
| `fetchTxResult` | Look up engine result by hash (`tx` RPC) |
| `waitForOutcome` | Poll until final result, LLS exceeded, or timeout — **never re-signs** |
| `fetchAccountObjectIds` | Generic object indexes |
| `rawRequest` | Send prebuilt JSON |
| `subscribeLedger` / `subscribeStreams` / `subscribeAccounts` / `subscribeAccountsProposed` / `subscribe` / `subscribeEx` | Ack only; pushes via stream handler |
| `unsubscribe*` / `unsubscribeEx` | Tear down subscriptions |
| `poll` | Drain WebSocket while subscribed (`WsXahauTransport` loops underneath) |

### Streams (subscribe push) — wallet / monitor path

Official reference: [subscribe](https://xahau.network/docs/features/http-websocket-apis/public-api-methods/).

```cpp
// Recommended wallet monitor: ledger + account in one round-trip
ws.setStreamHandler([](const char* json, size_t, void* user) {
  // json valid only during the callback; do NOT call request()/subscribe*
  xahau::StreamMessageType t;
  xahau::rpcParseStreamMessageType(json, t);
  if (t == xahau::StreamMessageType::LedgerClosed) {
    xahau::LedgerClosedEvent ev{};
    xahau::rpcParseLedgerClosedEvent(json, ev);
  } else if (t == xahau::StreamMessageType::Transaction) {
    xahau::TransactionStreamEvent ev{};
    xahau::rpcParseTransactionStreamEvent(json, ev);
  }
}, ctx);

ws.connect();
const char* streams[] = {"ledger"};
const char* accts[] = {myAddress};
xahau::SubscribeParams p{};
p.streams = streams; p.streamCount = 1;
p.accounts = accts; p.accountCount = 1;
// optional: p.accountsProposed = accts; p.accountsProposedCount = 1;
client.subscribeEx(p);
while (listening) client.poll();
client.unsubscribeEx(p);
```

#### Stream name matrix

| Stream | Push type | Clio public? | ESP32 note |
|--------|-----------|--------------|------------|
| `ledger` | `ledgerClosed` | Yes | **Preferred** for LLS / fee clock |
| `transactions` | `transaction` | Yes | High volume — avoid on device |
| `transactions_proposed` | `transaction` | Yes | High volume — prefer `accounts_proposed` |
| `validations` | `validationReceived` | Yes | Very high volume — avoid |
| `consensus` | `consensusPhase` | **No** (`reportingUnsupported`) | rippled only |
| `server` | `serverStatus` | **No** | rippled only |
| `manifests` | `manifestReceived` | Yes | Niche |
| `book_changes` | `bookChanges` | Yes | DEX summary |
| `peer_status` | `peerStatusChange` | Admin only | Not on public WSS |

#### Other subscribe fields

| Field | Library | Notes |
|-------|---------|-------|
| `accounts` | Yes | Validated txs affecting address(es) — **wallet P0** |
| `accounts_proposed` | Yes (`subscribeAccountsProposed` / `SubscribeParams`) | Pending + validated for address |
| `books` | Deferred | DEX builders not in MVP |
| `url` callbacks | Non-goal | Admin JSON-RPC |

#### Operational limits

- Stream handler receives a **NUL-terminated** copy into `streamScratch_` (**4096** bytes); larger frames are **truncated**.
- Do not call `request` / `subscribe*` from the handler (non-reentrant).
- `subscribe*` returns `Rejected` if ack contains `reportingUnsupported` (Clio).
- Struct event types may grow — **zero-init** before parse.
- Signing / encode paths are unchanged (additive network surface only).
- Examples: `examples/wss/Stream/LedgerStream`, `AccountStream`, `WalletMonitor`.

`SubmitResult`: `engineResult`, `txHash`, `tesSuccess`, `queued`, `validated`.  
Helpers: `rpcIsTesSuccess`, `rpcIsTerQueued`, `rpcEngineClass`.

### Multi-sign (`xahau/sign/multi_sign.h`)

```cpp
// 1) fill payment (or any tx) as usual
fillPayment(pay, accountId, pubKey, bag);
// 2) empty SigningPubKey
bagPrepareMultiSign(bag);
// 3) each signer
MultiSigner s1, s2;
multiSignAs(bag, keys1, s1);
multiSignAs(bag, keys2, s2);
// 4) combine → hex blob
MultiSigner all[] = {s1, s2};
multiSignFinalize(bag, all, 2, outHex, cap);
```

Loan dual-sign / Batch fill helpers were **removed** from library `src` on 2026-09-28 (XRPL-port EncodeOnly cut). Catalog `TxType` codes remain; see [ENCODE_ONLY_STUBS.md](ENCODE_ONLY_STUBS.md).

### Submit policy (production)

1. Prefer `xahau::sketch::submitAndWait` (or `submitBlob` + `waitForOutcome`).
2. **Never** re-sign with a new Sequence on `Status::Queued` or after `Timeout` if a hash was returned — resolve by hash first.
3. Identical blob re-submit is safe (idempotent); a **new** Sequence is a second payment.

### Transports

| Type | Use |
|------|-----|
| `MockXahauTransport` | Host tests / offline demos |
| `WsXahauTransport` | ESP32 WebSocket (`XAHAU_FEATURE_NET`) |

**WsXahauTransport (0.3.0+):** keeps one TLS/WS session (`setKeepAlive(true)` default), injects numeric JSON-RPC `id`, and matches response `id`. Call `close()` to force reconnect. Truncated responses are rejected (not partial-ok).

**TLS policy helper:**

```cpp
WsXahauTransport ws;
applyNetworkTransport(ws, networkFromIndex(XAHAU_NETWORK));  // Mainnet → fail-closed
// Mainnet only: also set fingerprint or CA
// ws.setSslFingerprint("AA:BB:...");
```

### Concurrency / device limits

| Resource | Policy |
|----------|--------|
| `signTx` on ESP32 | **Not re-entrant** — static BSS buffers by default. One sign at a time. Host builds use stack buffers. |
| `scratchSignBag()` | Shared static bag — one `buildAndSign*` / fill+sign path at a time. |
| `XahauRpcClient` response buffer | Default static 12 KiB — one client request at a time per process unless `setResponseBuffer`. |
| `WsXahauTransport` | One active callback pointer — do not call `request()` concurrently from two tasks. |

---

## Networks (`xahau/net/networks.h`)

| Preset | WSS | HTTPS JSON-RPC |
|--------|-----|----------------|
| `kNetworkMainnet` | xahau.network:**443** (NetworkID **21337**) | same |
| `kNetworkTestnet` | xahau-test.net:**443** (NetworkID **21338**) | same |
| *(no Devnet preset)* | — | Xahau has Mainnet+Testnet only in this library |

```cpp
WsXahauTransport ws(kNetworkTestnet);
// Mainnet hardening:
// ws.setSslFingerprint("AA:BB:...");
// ws.setRequireSslPin(true);
// ws.setInsecureSsl(false);
```

See [NETWORKS.md](NETWORKS.md).

---

## Feature flags (`xahau_platform.h`)

| Flag | Default | Purpose |
|------|---------|---------|
| `XAHAU_FEATURE_ED25519` | 1 | Ed25519 support |
| `XAHAU_FEATURE_SECP256K1` | 1 | secp256k1 support |
| `XAHAU_FEATURE_NET` | **1 on Arduino**, 0 on host | ESP32 WebSocket transport (`WsXahauTransport`) |
| `XAHAU_FEATURE_DEBUG` | 0 | Debug logging macros |

---

## Status codes (`Status`)

`Ok`, `InvalidArgument`, `BufferTooSmall`, `InvalidSeed`, `InvalidAddress`, `DecodeFailed`, `CryptoFailed`, `Timeout`, `Rejected`, `TransportFailed`, …

Use `statusName(Status)` for human-readable strings.

---

## Protocol field generation

```bash
python protocol/scripts/generate_fields.py
```

Writes `protocol/data/field_registry.json` and `src/xahau/generated/protocol_fields.h`.

---

## Examples

| Set | Path | Count |
|-----|------|------:|
| test (smoke) | `examples/test/` | 6 |
| rpc (offline) | `examples/rpc/` | 7 |
| wss (live) | `examples/wss/` | 25 |
| tx | `examples/tx/` | 35 |

See [EXAMPLES.md](EXAMPLES.md).
