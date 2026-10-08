# Test / smoke examples

Small sketches for offline checks and library smoke tests.

Full catalogs:

- [`../rpc/`](../rpc/) - offline JSON-RPC builders / mock (no Wi-Fi)
- [`../wss/`](../wss/) - live WebSocket (ESP32 + Wi-Fi)
- [`../tx/`](../tx/) - transaction builders (75)

| Sketch | Purpose | Network |
|--------|---------|---------|
| [`01_OfflineSignPayment`](./01_OfflineSignPayment/) | Offline Payment build/sign | none |
| [`02_DeriveKeys`](./02_DeriveKeys/) | Seed to address (Ed25519 / secp) | none |
| [`03_RpcMockSubmit`](./03_RpcMockSubmit/) | Mock transport (no Wi-Fi) | none |
| [`04_NetworkEndpoints`](./04_NetworkEndpoints/) | Print Mainnet / Testnet presets | none (print only) |
| [`05_MultiSignOffline`](./05_MultiSignOffline/) | Offline multi-sign Payment encode | none |
| [`06_GenerateSeed`](./06_GenerateSeed/) | Entropy -> family seed + address (fixed test entropy) | none |

Arduino IDE: **File -> Examples -> XahauClient -> test -> ...**
