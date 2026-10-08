# Locked decisions (2026-07-11)

| Decision | Choice |
|----------|--------|
| Location | `Documents\Arduino\libraries\XahauClient` |
| Integration | Arduino-first (`library.properties`, examples, `#include <XahauClient.h>`) |
| Testing | Native host tests via PlatformIO `native` env (no upload for codec/sign) |
| Curves | Ed25519 **and** secp256k1 from day one (API + feature flags) |
| Transactions | Full catalog of all user-submittable types + pseudo types registered |
| Relation to older prototypes / product apps | **Do not merge into this library**; apps depend on XahauClient |
| Wallet secrets | Never hardcoded in library; caller-owned keys/transport |
| License | MIT |

## Working agreement

- Build greenfield layers in order: codec → crypto → sign → typed fills → rpc → net.
- Raise each `TxType` maturity: Catalog → Encode → Validate → Golden.
- Prefer host golden vectors before testnet smoke.
