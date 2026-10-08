# XahauClient examples catalog

**Version:** 0.3.1 ship surface (**73** `.ino` sketches: test 6 + tx 35 + rpc 7 + wss 25).

All sketches under `examples/` use **placeholder** Wi-Fi (`YOUR_WIFI_*`). Seeds are `sEdPLACEHOLDER_*` in live tx sketches; destinations/issuers use `rYOUR_DESTINATION_ADDRESS` / `rYOUR_ISSUER_ADDRESS`. Prefer a gitignored `secrets.h` for real Wi-Fi and seeds (see [SECURITY.md](SECURITY.md)).  
Never commit real credentials.

**Scope:** live tree reflects types available on **Xahau mainnet/testnet**.
Non-Xahau / amendment-absent example folders were moved (not deleted) to harness
a maintainer archive. Library `src` EncodeOnly fill/sign helpers were **also removed** (catalog `TxType` codes only; see [ENCODE_ONLY_STUBS.md](ENCODE_ONLY_STUBS.md) and harness a maintainer archive).

Regenerate generated trees:

```bash
python protocol/scripts/generate_rpc_examples.py
python protocol/scripts/generate_examples.py
python protocol/scripts/reorganize_examples.py
```

---

## Test / smoke (6) - `examples/test/`

Offline and mock checks for install / board smoke testing.

| Path | Purpose |
|------|---------|
| `examples/test/01_OfflineSignPayment/` | Offline Payment build/sign |
| `examples/test/02_DeriveKeys/` | Seed -> address (Ed25519 / secp) |
| `examples/test/03_RpcMockSubmit/` | Mock transport (no Wi-Fi) |
| `examples/test/04_NetworkEndpoints/` | Print Mainnet / Testnet presets (NetworkID 21337/21338) |
| `examples/test/05_MultiSignOffline/` | Offline multi-sign Payment encode (no Wi-Fi) |
| `examples/test/06_GenerateSeed/` | Entropy -> family seed + address (fixed test entropy) |

Index: [examples/test/README.md](../examples/test/README.md)

---

## Transaction examples (35)

Grouped to match [Xahau Transaction Types](https://xahau.network/docs/protocol-reference/transactions/transaction-types/).

Each sketch: Wi-Fi + seed CONFIG, optional submit, offline blob mode via `DO_SUBMIT = false`.

### Accounts (6) - `examples/tx/Accounts/`

| Sketch | Type code |
|--------|----------:|
| AccountSet | 3 |
| AccountDelete | 21 |
| SetRegularKey | 5 |
| SignerListSet | 12 |
| TicketCreate | 10 |
| DepositPreauth | 19 |

### Payments (12) - `examples/tx/Payments/`

| Sketch | Type code |
|--------|----------:|
| Payment | 0 |
| EscrowCreate | 1 |
| EscrowFinish | 2 |
| EscrowCancel | 4 |
| CheckCreate | 16 |
| CheckCash | 17 |
| CheckCancel | 18 |
| PaymentChannelCreate | 13 |
| PaymentChannelFund | 14 |
| PaymentChannelClaim | 15 |
| PaymentMultiSign | 0 (multi-sign Payment demo) |
| Remit | **95** (Xahau-native; REMIT_MODE 0..4 multi-IOU / URITokenIDs / Mint) |

#### Remit `REMIT_MODE` 0..4 (`examples/tx/Payments/Remit/`)

Aligned with host goldens (`test_remit_uritoken` / `encode_shapes_secp.json`) and live Testnet Remit shapes. Sketch default is mode **2** (offline-safe); set `DO_SUBMIT=true` only on Testnet with a funded account.

| Mode | Shape | Live notes |
|-----:|-------|------------|
| 0 | XAH only (`amount` -> Amounts[0]) | Classic push; Destination funded |
| 1 | XAH + MintURIToken | Mint URI string in sketch; creates URIToken for Dest |
| 2 | XAH + USD IOU multi Amounts (`remit_xah_usd`) | Dest needs USD trust line; set `USD_ISSUER` |
| 3 | Small XAH + URITokenIDs (`remit_ids`) | Replace placeholder `URI_TOKEN_ID_HEX` with an ID you own |
| 4 | XAH + USD + URITokenIDs (`remit_xah_usd_uri`) | Richest; trust line + real URITokenID required |

Encode/sign works offline for all modes. Modes 2/4 need a Destination that can hold the IOU; modes 3/4 need a real 64-hex URITokenID before live submit.

### Tokens (7) - `examples/tx/Tokens/`

| Sketch | Type code |
|--------|----------:|
| TrustSet | 20 |
| URITokenMint | **45** (preferred NFT model on Xahau) |
| URITokenBurn | **46** |
| URITokenBuy | **47** |
| URITokenCreateSellOffer | **48** |
| URITokenCancelSellOffer | **49** |
| Clawback | 30 |

### DEX (2) - `examples/tx/DEX/`

| Sketch | Type code |
|--------|----------:|
| OfferCreate | 7 |
| OfferCancel | 8 |

### Decentralized Storage (2) - `examples/tx/DecentralizedStorage/`

| Sketch | Type code |
|--------|----------:|
| OracleSet | 51 |
| OracleDelete | 52 |

### Other (6) - `examples/tx/Other/`

| Sketch | Type code |
|--------|----------:|
| ClaimReward | **98** (Xahau-native) |
| SetRemarks | **94** (Xahau-native) |
| CronSet | **93** (Xahau-native; CRON_MODE unset) |
| Invoke | **99** (Hooks; non-BIP) |
| SetHook | **22** (Hooks; Tier A CreateCode; offline default) |
| Import | **97** (Xahau-native encode; not live until xPOP) |

**Not illustrated (by design):** Xahau pseudo-transactions `GenesisMint`, `EmitFailure`, `UNLReport`; catalog-only `LedgerStateFix`.

Index: [examples/tx/README.md](../examples/tx/README.md)

### Archived families (examples only)

See [ENCODE_ONLY_STUBS.md](ENCODE_ONLY_STUBS.md) and harness archive
a maintainer archive. Families removed from live examples tree:
NFToken*, MPToken*, DID*, Credential*, PermissionedDomain*, AMM*, Vault*/Loan*/LoanBroker*,
XChain*/SideChains/, Batch, DelegateSet.

---

## Offline RPC - `examples/rpc/` (7)

No Wi-Fi. Full builder/parser catalogs + mock client.

| Path | Purpose |
|------|---------|
| `rpc/BuildCatalog/` | **All** public `rpcBuild*` (account/server/submit/subscribe) using A+B seeds |
| `rpc/ParseCatalog/` | Major `rpcParse*` + stream event parsers (canned JSON) |
| `rpc/MockClientFlow/` | Mock: A->B Payment, submit, waitForOutcome, subscribe A+B |
| `rpc/BuildAccountInfo/` | Thin offline: `rpcBuildAccountInfo` (+ injection rejection). Live: `wss/Account/AccountInfo` |
| `rpc/BuildAccountObjects/` | Thin offline: `rpcBuildAccountObjects` (optional type filter). Live: `wss/Account/AccountObjects` |
| `rpc/BuildFeeAndSubmit/` | Thin offline: `rpcBuildFee` / `rpcBuildSubmit` / `rpcBuildSubmitEx`. Live submit: `wss/Transaction/SubmitBlob` |
| `rpc/ParseAccountInfo/` | Thin offline: `rpcParseAccountInfoSnapshot` on canned `account_info` JSON |

Thin sketches remain for IDE browsing; harness runs the catalogs.

Index: [examples/rpc/README.md](../examples/rpc/README.md)

## Live WSS (25) - `examples/wss/`

JSON-RPC over WebSocket to public clusters. Requires ESP32 + Wi-Fi.

Aligned with [Public API Methods](https://xahau.network/docs/features/http-websocket-apis/public-api-methods/).

### Server (7) - `examples/wss/Server/`

| Sketch | API / helper |
|--------|----------------|
| Ping | `ping` / `XahauRpcClient::ping` |
| Fee | `fee` / `fetchFeeDrops` |
| FeeAndLedger | `fetchFeeAndLedger` |
| ServerInfo | `server_info` |
| ServerState | `server_state` |
| LedgerClosed | `ledger_closed` |
| Ledger | `ledger` |

### Account (5) - `examples/wss/Account/`

| Sketch | API / helper |
|--------|----------------|
| AccountInfo | `account_info` / `fetchAccountInfo` |
| AccountLines | `account_lines` |
| AccountTx | `account_tx` |
| AccountObjects | `account_objects` |
| FetchSequenceAndLls | `fetchSequenceAndLls` |

### Transaction (4) - `examples/wss/Transaction/`

| Sketch | API / helper |
|--------|----------------|
| SubmitBlob | `submit` / `submitBlob` |
| SubmitFailHard | `submit` + `fail_hard` |
| TxLookup | `tx` |
| SubmitMultisigned | `submit_multisigned` (builder demo) |

### Transport (3) - `examples/wss/Transport/`

| Sketch | Purpose |
|--------|---------|
| RawRequest | `rawRequest` + builders |
| EndToEndPrepareSubmit | seq/fee -> sign Payment -> submit |
| NetworkPresets | endpoints + ping |

### Stream (3) - `examples/wss/Stream/`

| Sketch | Purpose |
|--------|---------|
| LedgerStream | `subscribeLedger` + `ledgerClosed` push handler |
| AccountStream | `subscribeAccounts` + transaction push handler |
| WalletMonitor | `subscribeEx` ledger + accounts (+ proposed) - product pattern |

### Catalog (3) - `examples/wss/Catalog/` (live RPC batteries)

| Sketch | Purpose |
|--------|---------|
| AccountRpcLive | Live account_* inventory RPCs (wallet lists) |
| LedgerServerRpcLive | Live ledger + server_* + feature/random |
| PathNftRpcLive | Live deposit_authorized + book_offers (primary); amm_info/nft_* often server-error on Xahau (prefer URIToken); A+B |

Index: [examples/wss/README.md](../examples/wss/README.md)

---

## Shared helpers

`src/xahau/sketch/example_runtime.h`

- `wifiConnect`
- `networkFromIndex` / `prepareCommon` / `prepareCommonForTx` / `submitAndLog`
- Hash / issuance ID parsers
- TLS policy notes (see [SECURITY.md](SECURITY.md))

### Special transaction costs

Some types must pay **>= 200000 drops** (owner reserve increment), not the ordinary open-ledger fee:

| Type | Min fee |
|------|--------:|
| `AccountDelete` | 200000 |

Library support:

- `xahau::txMinFeeDrops(TxType)` - table in `tx_type.h`
- `bagSetCommon` - **enforces** that floor when encoding `Fee`
- `prepareCommonForTx(client, keys, common, TxType::...)` - preferred for live sketches

---

## Totals

| Category | Count |
|----------|------:|
| Test / smoke | 6 |
| Offline RPC | 7 |
| Live WSS | 25 |
| Transactions | 35 |
| **All sketches** | **73** |
