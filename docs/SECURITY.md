# Security model

This document describes the threat model and mitigations for XahauClient.

## Trust boundaries

| Boundary | Assumption |
|----------|------------|
| **Seed / private key** | Held only on-device. Never sent by this library. Caller supplies seed **or** 16-byte CSPRNG entropy for generate; use `secureClear` after use. |
| **Seed generation** | Library encodes entropy only — **no RNG inside XahauClient**. Weak entropy is a catastrophic failure. Application must use hardware CSPRNG and **always** require the user to copy the seed offline before vault encryption (not recoverable later). |
| **Public RPC / WebSocket** | Server responses are **semi-trusted**. Parsers are lightweight; do not treat adversarial JSON as fully untrusted without extra validation. |
| **Caller-built strings** | Addresses, hex blobs, and type filters are **validated** before JSON is built. Multisign `tx_json` must be a JSON object the caller constructed. |
| **TLS** | SSL is used by default for public presets. Certificate **pinning is optional** but recommended for Mainnet. |

## Mitigations

### 1. JSON-RPC request building

- Classic account IDs: must start with `r`, base58 alphabet, length 3–40.
- `tx_blob` / transaction hashes: hex-only, even length.
- `ledger_index`: `current` | `validated` | `closed` | decimal digits.
- Type filters: `[A-Za-z0-9_]+`.
- String fields are JSON-escaped (`"`, `\`, controls as `\u00XX`).
- Invalid input returns `InvalidAddress` / `InvalidArgument` rather than broken JSON.

### 2. Signing buffers

- **Host:** stack-local buffers (re-entrant for concurrent host tests).
- **Arduino/ESP:** static BSS buffers by default (`XAHAU_SIGN_USE_STATIC_BUFS=1`) so
  `signTx` does not blow the ~8 KiB Arduino task stack alongside WebSockets.
- Signature material is wiped with `secureZero` before return.
- Caps (overridable): `XAHAU_SIGN_UNSIGNED_CAP`, `XAHAU_SIGN_SIGNED_CAP`, etc.
- On device, call `signTx` from one task at a time (or set `XAHAU_SIGN_USE_STATIC_BUFS=0`
  only if you increase the task stack enough).

### 3. TLS pinning (ESP32 `WsXahauTransport`)

```cpp
WsXahauTransport ws(kNetworkMainnet);
ws.setSslFingerprint("AA:BB:CC:...");  // form accepted by WebSocketsClient
// or:
ws.setSslCaCert(kRootCaPem);
ws.setRequireSslPin(true);   // fail closed if no pin/CA
ws.setInsecureSsl(false);    // refuse unpinned SSL
```

| Mode | Behavior |
|------|----------|
| Default | SSL on; unpinned allowed for public Testnet convenience |
| Fingerprint | `beginSSL(..., fingerprint)` |
| CA PEM | `beginSslWithCA(...)` preferred when set |
| Fail closed | `setRequireSslPin(true)` without pin/CA → `InvalidArgument` |

Certificate fingerprints change when operators rotate certs. Prefer CA pins for long-lived firmware, or ship fingerprint updates.

## Secrets hygiene

| Location | Policy |
|----------|--------|
| Examples | Wi-Fi: `YOUR_WIFI_*` only. Seeds: `sEdPLACEHOLDER_*`. Destinations/issuers: `rYOUR_DESTINATION_ADDRESS` / `rYOUR_ISSUER_ADDRESS`. Never use mainnet seeds in examples. |
| Local device flashes | Put real Wi-Fi / seeds in a **gitignored** `secrets.h` (or equivalent) — never commit |
| Host tests / vectors | Committed tree uses placeholders. Real fixtures live only in gitignored `test/native/test_seeds.local.h` (copy from `test_seeds.example.h`). Do **not** fund fixtures with real value |
| Live Testnet harness | Out of tree (e.g. ClientTest `secrets.json`) — **never** commit into this library |
| Application firmware | Load seeds from secure storage; never hardcode mainnet keys |

Without `test_seeds.local.h`, seed-dependent host tests skip cleanly; encode-only tests still run. Never deposit real mainnet value into test fixtures.

## What this library does not provide

- Hardware secure element / TEE key storage
- Full JSON Schema validation of server responses
- Automatic Mainnet spend limits or confirmation UX
- Threshold multi-sign beyond building `SignerListSet` / multisign envelopes

## Submit / re-sign policy (double-spend footgun)

Public clusters often return **`terQUEUED`** or transport **`Timeout`** after the blob has already been accepted. Treat these as ambiguous, not hard failures:

1. Use `waitForOutcome(hash, lastLedgerSequence, …)` or sketch `submitAndWait`.
2. **Never** re-prepare Sequence + re-sign a *new* blob until the original hash is final or past LLS.
3. Re-submitting the **identical** signed blob is safe (idempotent). A new Sequence is a second transaction.

## WebSocket streams (subscribe)

- Unsolicited TEXT frames are copied into a fixed **`streamScratch_` (4096 bytes)** then delivered to `setStreamHandler`. Oversized pushes are **truncated** mid-JSON; parsers must tolerate partial input.
- **Never** call `request()`, `rawRequest()`, or `subscribe*` from inside the stream handler (re-entrancy / stack risk).
- Prefer **`accounts` + `ledger`** over global `transactions` / `validations` on ESP32 (bandwidth + CPU).
- Stream names are allowlisted; accounts Base58Check-validated. Admin `url` subscribe callbacks are not implemented.
- Clio public clusters reject `server`, `consensus`, `peer_status` (`reportingUnsupported` → client `Rejected`).

## Residual risks (accepted for embedded clients)

1. Testnet default TLS remains unpinned for convenience. **Mainnet** uses `applyDefaultTlsPolicy` → `requireSslPin=true` (fail closed without fingerprint/CA).
2. `WsXahauTransport` uses a single active callback pointer — do not call `request()` concurrently from two tasks.
3. RPC response parsing is substring-based and assumes a cooperative ledger server.
4. Classic account IDs in RPC builders are **Base58Check**-validated (Wave D); still semi-trusted JSON from the cluster.
5. Stream parsers do not fully validate adversarial JSON; treat public cluster pushes as semi-trusted.

## Reporting

Treat this as an embedded wallet SDK. Issues that leak key material, weaken signature uniqueness, or allow remote transaction injection via crafted RPC fields are high priority.

Report privately to the maintainer in `library.properties` when appropriate.
