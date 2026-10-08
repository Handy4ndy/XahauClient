# XahauClient

**Version:** 0.3.1

Arduino / ESP32 client for the **Xahau** ledger — forked and adapted from XRPLClient to match **live Xahau mainnet** protocol rules and transaction types.

## Why this library exists

Xahau shares XRPL DNA (addresses, Ed25519/secp256k1, STObject codec) but is a **separate L1**:

| | XRPL | Xahau |
|--|------|-------|
| Native token | XRP | **XAH** |
| On-wire NetworkID | omit on main/test/dev | **required** (21337 / 21338) |
| NFT model | NFToken | **URIToken** |
| Smart contracts | — | **Hooks** (`SetHook`, `Invoke`, emit) |
| Oracles | PriceOracle (type 51/52) | **PriceOracle (type 60/61)** — enabled mainnet |
| Multi-pay | — | **Remit** |
| Rewards | — | **ClaimReward** |

## Supported hardware

- **ESP32** and **ESP32-S3** (Arduino core) for networked examples (Wi-Fi + WebSockets)
- Offline encode/sign sketches compile on the same boards without Wi-Fi
- Host (native) unit tests via PlatformIO `pio test -e native` (developer machines)

Depends on **WebSockets** by Markus Sattler (Links2004) for ESP32 WebSocket transport.

## Install

### Arduino Library Manager (when listed)

1. **Sketch → Include Library → Manage Libraries…**
2. Search **XahauClient** → Install
3. Install **WebSockets** (Links2004) if prompted
4. Restart the IDE → **File → Examples → XahauClient**

Until the library is listed, use ZIP, git clone, or PlatformIO below.

### Arduino IDE — ZIP

1. Download the release ZIP (or zip the `XahauClient` folder)
2. **Sketch → Include Library → Add .ZIP Library…**
3. Install **WebSockets** via Library Manager
4. Restart the IDE → Examples → XahauClient

### Arduino IDE — folder / git clone

1. Place or clone into your sketchbook libraries directory, e.g. `Documents/Arduino/libraries/XahauClient`
2. Install **WebSockets** (Library Manager)
3. Restart Arduino IDE → Examples → XahauClient

### PlatformIO

```ini
lib_deps =
  https://github.com/Handy4ndy/XahauClient.git#v0.3.1
  links2004/WebSockets
```

Or a local checkout / symlink under `lib/`. Registry install (when published): `lib_deps = XahauClient` (or `owner/name`). Always also declare `links2004/WebSockets` for ESP32 WSS sketches.

## Quick start

### 1) Offline smoke (no Wi-Fi)

Open **Examples → XahauClient → test → 01_OfflineSignPayment**.

1. Replace `XAHAU_SEED` with a **Testnet** family seed only (never a mainnet seed in shared code)
2. Compile / upload; Serial prints a signed Payment hex blob

### 2) Live Testnet Payment (ESP32 / ESP32-S3)

Open **Examples → XahauClient → tx → Payments → Payment**  
(or **wss → Transport → EndToEndPrepareSubmit**).

1. Set Wi-Fi (`YOUR_WIFI_SSID` / `YOUR_WIFI_PASSWORD`) and a funded Testnet `XAHAU_SEED` (examples ship `sEdPLACEHOLDER_*`)
2. Replace destination / issuer placeholders (`rYOUR_DESTINATION_ADDRESS`, `rYOUR_ISSUER_ADDRESS`) with funded Testnet accounts
3. Prefer a gitignored local `secrets.h` (or equivalent) for real Wi-Fi and seeds — never commit them
4. `XAHAU_NETWORK = 1` (testnet) or `0` (mainnet); set `DO_SUBMIT = true` when you intend to submit
5. Flash the board, open Serial **115200**

The sketch sets **`TxCommon.networkId`** from the network preset (required on Xahau), fetches Sequence / LLS / fee over WSS, signs with the **STX\\0** prefix, and submits `tx_blob`.

More guided paths: [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md) · full catalog: [docs/EXAMPLES.md](docs/EXAMPLES.md)

## Networks

| Preset | Host | Protocol NetworkID |
|--------|------|--------------------|
| Mainnet | `xahau.network:443` | **21337** |
| Testnet | `xahau-test.net:443` | **21338** |

WSS example: `wss://xahau-test.net` (port 443). Always apply NetworkID before signing:

```cpp
xahau::sketch::applyNetworkId(params.common, net);
```

## Examples (73 sketches)

| Group | Count | Notes |
|-------|------:|-------|
| `examples/test/` | 6 | Offline keys, sign, multi-sign, seed generation |
| `examples/rpc/` | 7 | Offline RPC builders / parsers / mock client |
| `examples/wss/` | 25 | Live WebSocket: account, server, tx, streams, catalog |
| `examples/tx/` | 35 | Transaction fills: Payments, Tokens, DEX, Hooks, Oracle, … |

See [docs/EXAMPLES.md](docs/EXAMPLES.md) for the full list.

## Transaction coverage (0.3.1)

**Submit-ready / encode+sign on public Xahau (typical):** Payment, TrustSet, Offers, Escrow, Checks, PayChan, Tickets, RegularKey, multi-sign, Clawback, Hooks (`SetHook` / `Invoke`), URIToken market, Remit, ClaimReward, CronSet, SetRemarks, OracleSet / OracleDelete, and related classics.

**Known limitations:**

- **Import** — encode is present; **live submit needs a valid xPOP** fixture
- **XChain\*** — **not supported** (catalog type codes only; no fill/sign in `src`)
- **DID / Credential / MPT / PermissionedDomain** — encode helpers **pending** (amendments not enabled on public Xahau); catalog codes only
- **NFToken\* / AMM\* / Vault / Loan / Batch / DelegateSet** — catalog only; prefer **URIToken** for NFTs on Xahau

Details: [docs/REMAINING_GAPS.md](docs/REMAINING_GAPS.md), [docs/ENCODE_ONLY_STUBS.md](docs/ENCODE_ONLY_STUBS.md).

## Include

```cpp
#define XAHAU_FEATURE_NET 1
#include <XahauClient.h>
#include <xahau/sketch/example_runtime.h>
```

## Docs

| Doc | What |
|-----|------|
| [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md) | Install + first sketches |
| [docs/EXAMPLES.md](docs/EXAMPLES.md) | Full catalog (73 INOs) |
| [docs/API.md](docs/API.md) | Public C++ API surface |
| [docs/NETWORKS.md](docs/NETWORKS.md) | Mainnet 21337 / Testnet 21338 |
| [docs/SECURITY.md](docs/SECURITY.md) | Seeds, TLS, secrets policy |
| [docs/REMAINING_GAPS.md](docs/REMAINING_GAPS.md) | Intentional opens |
| [docs/LIBRARY_ROADMAP.md](docs/LIBRARY_ROADMAP.md) | Ship surface + near-term |
| [docs/XAHAU_VS_XRPL.md](docs/XAHAU_VS_XRPL.md) | Protocol differences vs XRPL |
| [docs/PUBLISHING.md](docs/PUBLISHING.md) | Release checklist |
| [docs/README.md](docs/README.md) | Docs index |

## Testing (host)

Host Unity tests: `pio test -e native` (MinGW on Windows).

Seed-dependent fixtures are **not** committed. Copy the template and fill local values:

```bash
cp test/native/test_seeds.example.h test/native/test_seeds.local.h
```

`test_seeds.local.h` is gitignored. Without it, seed-dependent cases **skip** cleanly (encode-only tests still run — typically **61 pass / 62 skip**). With a filled `test_seeds.local.h`, the full suite should be **123/123**.

Never commit real seeds, Wi-Fi credentials, or private keys. See [docs/SECURITY.md](docs/SECURITY.md) and [CONTRIBUTING.md](CONTRIBUTING.md).

## License

**MIT** — see [LICENSE](LICENSE) and [NOTICE](NOTICE). Vendor crypto retains original notices under `src/xahau/crypto/vendor/`.
