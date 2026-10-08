# XahauClient WSS examples (live WebSocket)

Complete set of ESP32 sketches for every **wired JSON-RPC builder** and
**XahauRpcClient** helper, over public WebSocket endpoints.

Companion to transaction examples under [`../tx/`](../tx/).

## Xahau public API map

Aligned with [Public API Methods](https://xahau.network/docs/features/http-websocket-apis/public-api-methods/):

| Group | Path | Sketches | Library coverage |
|-------|------|---------:|------------------|
| Server | `wss/Server/` | 7 | ping, fee, server_info, server_state, ledger, ledger_closed |
| Account | `wss/Account/` | 5 | account_info, account_lines, account_tx, account_objects |
| Transaction | `wss/Transaction/` | 4 | submit, submit fail_hard, tx, submit_multisigned |
| Transport | `wss/Transport/` | 3 | rawRequest, e2e submit, network presets |
| Stream | `wss/Stream/` | 3 | subscribe ledger + accounts + wallet monitor |
| Catalog | `wss/Catalog/` | 3 | **live** wallet-priority RPC batteries (account / ledger / path+nft) |

**Total: 25 sketches** (Vault/Loan object helpers archived with non-Xahau Lending examples).

## How to use

1. Open `examples/wss/<Group>/<Name>/`.
2. Set `WIFI_SSID`, `WIFI_PASSWORD`, `XAHAU_NETWORK`.
3. For account methods: set `ACCOUNT` **or** a funded `XAHAU_SEED`.
4. Flash ESP32 (WebSockets library required).

```cpp
const char* WIFI_SSID    = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const int   XAHAU_NETWORK = 1;  // testnet
const char* ACCOUNT      = ""; // or r... ; empty uses seed address
const char* XAHAU_SEED    = "sEd...";
```

## Flow

```text
WiFi -> WsXahauTransport(kNetwork*) -> XahauRpcClient
     -> rpcBuild* / client.fetch* / client.submitBlob / rawRequest
     -> Serial print

Stream:
WiFi -> setStreamHandler -> connect -> subscribe* -> poll() -> unsubscribe*
```

## Offline builders (no WiFi)

Pure `rpcBuild*` / mock client sketches: [`../rpc/`](../rpc/).
Host tests: `pio test -e native -f test_rpc`.

## Related

- Offline RPC: [`../rpc/`](../rpc/)
- Tx examples: [`../tx/`](../tx/)
- Smoke tests: [`../test/`](../test/)
- Networks: [`../../docs/NETWORKS.md`](../../docs/NETWORKS.md)
- Security / TLS pin: [`../../docs/SECURITY.md`](../../docs/SECURITY.md)

## Regenerate

```bash
python protocol/scripts/generate_wss_examples.py
```
