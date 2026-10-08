# Getting started with XahauClient

## Requirements

| Item | Notes |
|------|--------|
| Arduino IDE 2.x or PlatformIO | ESP32 core for live Wi-Fi sketches |
| ESP32 board | Required for WSS submit examples |
| [WebSockets](https://github.com/Links2004/arduinoWebSockets) | Required on ESP32 (listed in `library.properties` `depends`) |
| Optional: PlatformIO + MinGW (Windows) | Host unit tests without a board |

## Install the library

### Arduino IDE

1. Place the `XahauClient` folder under your sketchbook libraries directory  
   (typically `Documents/Arduino/libraries/XahauClient`).
2. Restart the IDE.
3. Confirm **File -> Examples -> XahauClient** lists examples.

### PlatformIO

```ini
lib_deps =
  https://github.com/Handy4ndy/XahauClient.git#v0.3.1
  links2004/WebSockets
```

Or a local path / symlink under `lib/`. Registry install (when published): `lib_deps = XahauClient` (or `owner/name` once registered).
See root [README.md](../README.md) Install for ZIP / folder steps.

## Your first offline sketch

1. Open **Examples -> XahauClient -> test -> 01_OfflineSignPayment**.
2. Replace `XAHAU_SEED` with a **Testnet** family seed only  
   (never a mainnet seed in shared code).
3. Upload or run; Serial prints a signed hex blob or an error if the seed is invalid.

Keys work like JS/Python:

```cpp
xahau::deriveKeysFromSeed("sEd...", keys);  // Ed25519
xahau::deriveKeysFromSeed("s...", keys);    // secp256k1
```

## First live Testnet sketch

1. Create a Testnet account via the [Xahau Testnet faucet](https://docs.xahau.network/) (or your usual Testnet faucet).
2. Open **Examples -> XahauClient -> wss -> Transport -> EndToEndPrepareSubmit**  
   (or **tx -> Payments -> Payment**).
3. Set:

```cpp
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* XAHAU_SEED     = "s...";   // funded Testnet seed
const int   XAHAU_NETWORK  = 1;      // Testnet
const bool  DO_SUBMIT     = true;   // Payment sketch
// Also replace rYOUR_DESTINATION_ADDRESS / rYOUR_ISSUER_ADDRESS in tx sketches.
// Prefer a gitignored secrets.h for Wi-Fi and seeds.
```

4. Install **WebSockets** (Links2004) if Arduino prompts, or via Library Manager.
5. Flash ESP32 and open Serial Monitor at 115200 baud.

### Linker error: `undefined reference to WsXahauTransport`

Arduino compiles library `.cpp` files **without** sketch `#define`s. XahauClient
defaults `XAHAU_FEATURE_NET=1` on Arduino so `ws_transport.cpp` is linked. If you
still see this error:

1. Confirm library is `Documents/Arduino/libraries/XahauClient` (v0.3.1).
2. Install the **WebSockets** library (Sketch -> Include Library -> Manage Libraries).
3. Restart the IDE / recompile after updating the library files.

### Crash on boot: `Guru Meditation` / `LoadProhibited`

Usually **stack overflow**. Do **not** put `WsXahauTransport` on the `setup()` stack
(`WebSocketsClient` is large; default Arduino task stack is ~8 KiB).

Examples use:

```cpp
static xahau::WsXahauTransport s_ws;   // BSS, not stack
static xahau::XahauRpcClient s_client(s_ws);
```

Library-side: `signTx` and the default RPC response buffer also use static BSS on
Arduino for the same reason.

## Which example should I open?

| Goal | Example |
|------|---------|
| Offline Payment only | `examples/test/01_OfflineSignPayment` |
| Derive address from seed | `examples/test/02_DeriveKeys` |
| RPC without Wi-Fi | `examples/test/03_RpcMockSubmit` |
| Print network hosts/ports | `examples/test/04_NetworkEndpoints` (Mainnet + Testnet) |
| Live ping | `examples/wss/Server/Ping` |
| Live account balance / seq | `examples/wss/Account/AccountInfo` |
| Offline RPC builders | `examples/rpc/BuildAccountInfo` |
| Live Payment | `examples/tx/Payments/Payment` or `wss/Transport/EndToEndPrepareSubmit` |
| URIToken market (Xahau NFT) | `examples/tx/Tokens/URITokenMint` (+ Buy/Burn/CreateSell/CancelSell) |
| Remit (Xahau-native) | `examples/tx/Payments/Remit` (`REMIT_MODE` 0..4 - see EXAMPLES.md) |
| Import encode (needs xPOP for live) | `examples/tx/Other/Import` - see ENCODE_ONLY_STUBS.md for archived XRPL-port Lending/AMM/NFToken/... |

Full list: [EXAMPLES.md](EXAMPLES.md).

## Networks

| `XAHAU_NETWORK` | Network | Typical use |
|----------------|---------|-------------|
| `0` | Mainnet | Real value - double-check every field |
| `1` | Testnet | Default for examples |
| `2+` | Testnet | Same as `1` (`networkFromIndex`; no separate Devnet) |

Details: [NETWORKS.md](NETWORKS.md).

## Common failures

| Symptom | Check |
|---------|--------|
| `deriveKeysFromSeed` fails | Seed must be valid base58 family seed (`sEd...` or `s...`) |
| Wi-Fi timeout | SSID/password; 2.4 GHz on most ESP32 boards |
| WSS timeout | Firewall; Xahau hosts use **443** (`xahau.network` / `xahau-test.net`) |
| `actNotFound` | Account not funded on that network |
| `tec*` / `tem*` | Sequence, fee, destination, or amendment missing |

## Next steps

- API reference: [API.md](API.md)
- Example catalog (**73** INOs): [EXAMPLES.md](EXAMPLES.md)
- Intentional opens (0.3.1 ship): [REMAINING_GAPS.md](REMAINING_GAPS.md)
- Ship surface / near-term: [LIBRARY_ROADMAP.md](LIBRARY_ROADMAP.md)
- Version: see `library.properties` / `library.json` (currently 0.3.1)
- Security practices: [SECURITY.md](SECURITY.md)
- Host tests: `pio test -e native` from the library root
- Full docs index: [README.md](README.md)
