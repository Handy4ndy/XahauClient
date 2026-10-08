# Architecture

## Principles

1. **One serialization path** — every transaction becomes a `TxFieldBag`, then `xahauSerialize` + `signTx`. No per-tx hand-rolled binary emitters that diverge.
2. **Core is portable** — no `String`, `Serial`, Wi-Fi, or NVS in `codec` / `crypto` / `sign`. Arduino is a consumer platform, not a core dependency.
3. **Both curves from day one** — Ed25519 (`sEd…`) and secp256k1 (classic `s…`); signing selects the algorithm from key material.
4. **Full catalog** — all Xahau user-submittable transaction types exist in `TxType` + registry. Maturity is per-type (`Catalog` → `Encode` → `Validate` → `Golden`).
5. **Network is optional** — RPC JSON builders are pure string logic; sockets live behind `IXahauTransport`.
6. **Test on host** — codec and signing are proven with Unity on `platform = native` before any flash.

## Layers

```text
Application / sketch
        |
        v
+-------------------+
|  Public API       |  XahauClient.h, buildAndSign*, XahauRpcClient
+---------+---------+
          |
   +------+------+
   v             v
+------+    +-----------+
| tx/  |    | rpc + net |
| fills|    | transport |
+--+---+    +-----------+
   v
+----------------+
| sign + codec   |  bag -> bytes -> STX -> signature -> hex
+--------+-------+
         v
+----------------+
| crypto         |  seed, base58, Ed25519, secp + BIP66
+--------+-------+
         v
+----------------+
| generated/     |  protocol field codes
+----------------+
```

## Transaction lifecycle

```text
Typed params (PaymentParams, ...)
        |  fillPayment / buildAndSignPayment
        v
   TxFieldBag   (fieldCode -> value slots, fixed arena)
        |  xahauSerialize(bag, includeSigningFields=false)
        v
   unsigned STTx bytes
        |  prefix STX\0 + Ed25519 or secp256k1
        v
   signature
        |  bag + TxnSignature -> serialize true
        v
   tx_blob hex  ->  submit RPC (optional)
```

## Registry

Each `TxType` has a name, wire code, maturity, and (usually) a typed filler.  
Signing never switches on transaction type — only on the bag contents and key curve.

## Protocol tables

Field codes are generated from Xahau protocol definitions into:

- `protocol/data/field_registry.json`
- `src/xahau/generated/protocol_fields.h`

```bash
python protocol/scripts/generate_fields.py
```

## Feature flags

| Flag | Default | Purpose |
|------|---------|---------|
| `XAHAU_FEATURE_ED25519` | 1 | Ed25519 |
| `XAHAU_FEATURE_SECP256K1` | 1 | secp256k1 |
| `XAHAU_FEATURE_NET` | 1 on Arduino / 0 on host | ESP32 WebSocket (must match when compiling library `.cpp`) |
| `XAHAU_FEATURE_DEBUG` | 0 | Debug logging |

## Examples packaging

| Tree | Role |
|------|------|
| `examples/test/` | Offline / mock smoke sketches |
| `examples/rpc/` | Offline JSON-RPC builders, parsers, Mock client |
| `examples/wss/` | Live WebSocket (JSON-RPC over WSS) |
| `examples/tx/<Group>/` | One sketch per wired transaction (Xahau feature groups) |

Generators live under `protocol/scripts/`.

## Non-goals

- Full wallet UI, vault product, or NVS keystore (application responsibility)
- Admin RPC methods
- Replacing Clio / full-history indexing
