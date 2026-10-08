# XahauClient examples

Four top-level sections (Xahau mainnet/testnet live tree — **0.3.1** / 2026-09-28):

| Section | Path | Count | Role |
|---------|------|------:|------|
| **test** | [`test/`](test/) | 6 | Install / board smoke (offline + mock + presets print) |
| **rpc** | [`rpc/`](rpc/) | 7 | JSON-RPC builders, parsers, Mock client — **no Wi-Fi** |
| **wss** | [`wss/`](wss/) | 25 | Live WebSocket to public clusters — **ESP32 + Wi-Fi** |
| **tx** | [`tx/`](tx/) | 35 | Transaction encode/sign (and optional live submit) |

**All sketches: 73**

The published tree is **Xahau mainnet/testnet only**. Former XRPL-port / amendment-absent
example folders are not shipped in this package (see [docs/ENCODE_ONLY_STUBS.md](../docs/ENCODE_ONLY_STUBS.md)).

**Why split rpc vs wss?**  
On Xahau, client APIs are **JSON-RPC**. **WSS** is how this library carries them on-device
(`WsXahauTransport`). Offline code (builders/parsers/mock) does not need a socket; live
ledger I/O does. Keeping them separate matches that layering.

| Guide | Link |
|-------|------|
| Full catalog | [docs/EXAMPLES.md](../docs/EXAMPLES.md) |
| Getting started | [docs/GETTING_STARTED.md](../docs/GETTING_STARTED.md) |
| Security | [docs/SECURITY.md](../docs/SECURITY.md) |

---

## test — smoke

See **[test/README.md](test/README.md)**.

| Sketch | Purpose |
|--------|---------|
| `01_OfflineSignPayment` | Offline Payment sign |
| `02_DeriveKeys` | Seed → address |
| `03_RpcMockSubmit` | Minimal mock transport |
| `04_NetworkEndpoints` | Print network presets |
| `05_MultiSignOffline` | Offline multi-sign Payment |
| `06_GenerateSeed` | Entropy → family seed |

---

## rpc — offline JSON-RPC

See **[rpc/README.md](rpc/README.md)**.

Builders, parsers, and `MockXahauTransport` client flows. No Wi-Fi.

---

## wss — live WebSocket

See **[wss/README.md](wss/README.md)**.

| Group | Path | Count |
|-------|------|------:|
| Server | `wss/Server/` | 7 |
| Account | `wss/Account/` | 5 |
| Transaction | `wss/Transaction/` | 4 |
| Transport | `wss/Transport/` | 3 |
| Catalog | `wss/Catalog/` | 3 |
| Stream | `wss/Stream/` | 3 |

Requires ESP32, Wi-Fi, WebSockets library.

---

## tx — transactions

See **[tx/README.md](tx/README.md)** and [docs/EXAMPLES.md](../docs/EXAMPLES.md).

| Group | Count |
|-------|------:|
| Accounts | 6 |
| Payments | 12 |
| Tokens | 7 |
| DEX | 2 |
| DecentralizedStorage | 2 |
| Other | 6 |
