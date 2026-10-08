# Networks — Xahau Mainnet / Testnet

## Important

**Encode and sign do not depend on network.** Any filled transaction can be serialized offline.

**Live submit** only succeeds if the target network has the required amendments enabled and your account is funded there.

Every signed Xahau transaction **must** include on-wire **NetworkID** (`>= 1025`):

| Network | Protocol NetworkID |
|---------|-------------------|
| Mainnet | **21337** |
| Testnet | **21338** |

Use `sketch::applyNetworkId(common, ep)` (or set `TxCommon.networkId` yourself). `bagSetCommon` emits field `NetworkID` when `networkId >= 1025`.

Features are **never removed** from the library because an amendment is off somewhere. Use `isTxLiveEnabledOn` / `amendmentStatus` / `txLiveSupport` to decide whether to run live smoke.

## Official public endpoints (library presets)

Aligned with `src/xahau/net/networks.h` (not XRPL public clusters):

| Network | WebSocket (JSON-RPC) | JSON-RPC HTTPS | Protocol NetworkID |
|---------|----------------------|----------------|--------------------|
| **Mainnet** | `wss://xahau.network:443/` | `https://xahau.network:443/` | **21337** |
| **Testnet** | `wss://xahau-test.net:443/` | `https://xahau-test.net:443/` | **21338** |

There is **no separate public Xahau Devnet preset** in this library. Example index `XAHAU_NETWORK`: `0` = mainnet, anything else = testnet (`networkFromIndex`).

> Do **not** point this client at `xrplcluster.com` / `*.rippletest.net` — those are XRPL hosts.

### Subscribe streams

Prefer `streams: ["ledger"]` + `accounts: [r…]` on ESP32. Avoid global `transactions` / `validations` unless you have RAM and a clear need.

## C++ presets

```cpp
#include <XahauClient.h>

const NetworkEndpoint& ep = kNetworkTestnet;  // or kNetworkMainnet
// WsXahauTransport ws(ep);   // XAHAU_FEATURE_NET=1
// XahauRpcClient client(ws);

networkPreset(NetworkId::Testnet);
networkPresetByName("testnet");
protocolNetworkIdFor(NetworkId::Testnet);  // 21338

allPublicNetworksWired();
networkEndpointValid(ep);
```

## Amendment matrix (Xahau, snapshot)

Live status is driven by `networks.h` + periodic `feature` RPC. High-level:

| Class | Meaning | Examples |
|-------|---------|----------|
| **Enabled** | Live on Mainnet / Testnet | Hooks, URIToken, Remit, Import, Cron, Remarks, ClaimReward, Oracle, Clawback, … |
| **NotEnabled** | In definitions / release but not enabled | NamedHooks, HooksUpdate2, HookOnV2, AMM*, AMMClawback |
| **Absent** | Not in Xahau testnet `feature` list (2026-09-26) | DID, Credentials, MPTokensV1, PermissionedDomains |

XRPL-only port stubs (Vault/Loan/Batch/DelegateSet, codes 200+) stay **encode-only** — do not submit on Xahau.

NFToken* types are catalog/encode-only on Xahau; **URIToken** is the live NFT model.

Re-query after upgrades:

```text
POST {"method":"feature","params":[{}]}
```

### APIs

```cpp
amendmentStatus(AmendmentId::URIToken, NetworkId::Testnet);  // Enabled
amendmentStatus(AmendmentId::DID, NetworkId::Testnet);       // Absent
isTxLiveEnabledOn(TxType::URITokenMint, NetworkId::Testnet); // true
isTxLiveEnabledOn(TxType::DIDSet, NetworkId::Testnet);       // false
isTxLiveEnabledOn(TxType::VaultCreate, NetworkId::Testnet);  // false
txLiveSupport(TxType::NFTokenModify);                        // EncodeOnly
txLiveSupport(TxType::Payment);                              // AllPublic
recommendedLiveNetwork(TxType::Payment);                     // Testnet
```

## Coarse live-support classes

| Class | Meaning | Examples |
|-------|---------|----------|
| **AllPublic** | Live on Mainnet / Testnet when amendment enabled | Payment, Remit, URIToken*, CronSet, SetRemarks, Invoke, Oracle, Clawback, … |
| **EncodeOnly** | Encode OK; do not expect live success | NFToken*, AMM*, DID*, Credential*, MPT*, PermissionedDomain*, Batch, Vault*, Loan*, … |
| **NotUserSubmit** | Network / pseudo txs | EnableAmendment, SetFee, UNLModify, Cron, GenesisMint, EmitFailure, UNLReport |

## Wiring checklist

| Item | Mainnet | Testnet |
|------|---------|---------|
| Preset | `kNetworkMainnet` | `kNetworkTestnet` |
| WS / RPC | `xahau.network:443` | `xahau-test.net:443` |
| NetworkID | **21337** | **21338** |
| Faucet hint | none | ecosystem / Xaman tools |
| Offline encode all catalog txs | yes | yes |
| Live URIToken / Remit / Cron / Hooks | **yes** | **yes** |
| Live DID / MPT / Credential / Domain | **no** (absent) | **no** (absent) |
| Live NFToken* / Vault / Loan / Batch | **no** | **no** |

## ESP32 example

```cpp
#define XAHAU_FEATURE_NET 1
#include <XahauClient.h>
#include <xahau/net/ws_transport.h>
#include <xahau/sketch/example_runtime.h>

WsXahauTransport ws(kNetworkTestnet);
XahauRpcClient client(ws);
TxCommon common;
applyNetworkId(common, kNetworkTestnet);

if (!xahau::isTxLiveEnabledOn(TxType::URITokenMint, NetworkId::Testnet)) {
  // skip live submit — still may encode offline
}
```

## Host tests

```text
pio test -e native -f test_networks
```

Validates hosts, NetworkIDs, and live-support coverage — **no network I/O**.

## Faucets

- Testnet: ecosystem faucets / Xaman (see `kNetworkTestnet.faucetHint`)

## Related

- [XAHAU_VS_XRPL.md](XAHAU_VS_XRPL.md) — NetworkID and type-code deltas vs XRPL
- [REMAINING_GAPS.md](REMAINING_GAPS.md) — intentional opens (Import / SetHook serial / XChain / P1 absent)
- [ENCODE_ONLY_STUBS.md](ENCODE_ONLY_STUBS.md) — catalog-only families removed from `src`
