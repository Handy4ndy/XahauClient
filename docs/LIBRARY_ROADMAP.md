# Library roadmap

Status for **XahauClient 0.3.1**.

| Fact | Value |
|------|------:|
| Library version | **0.3.1** |
| Example `.ino` sketches | **73** (test 6 + rpc 7 + wss 25 + tx 35) - Xahau mainnet/testnet only |
| Native host suite | **123** / 123 (`pio test -e native`) |
| Live testnet coverage (21338) | Closed for the 0.3.1 ship surface |
| EncodeOnly XRPL-port `src` cut | See [ENCODE_ONLY_STUBS.md](ENCODE_ONLY_STUBS.md) |

Companion docs:

- [ROADMAP.md](ROADMAP.md) - historical phase roadmap
- Version history is tracked in git tags / release notes (no CHANGELOG file in-tree)
- [EXAMPLES.md](EXAMPLES.md) - full sketch catalog
- [REMAINING_GAPS.md](REMAINING_GAPS.md) - intentional opens

---

## Current (0.3.1 ship surface)

- [x] Optional-auth closed on Xahau testnet **21338**
- [x] Xahau-native live where proven: CronSet, SetRemarks, Remit (+ MintURIToken / rich Amounts), URIToken market, ClaimReward, Invoke / SetHook encode (+ live where recorded)
- [x] Example pack **73** INOs - Xahau-only
- [x] Native host suite **123** cases after EncodeOnly `src` cut
- [x] Docs / endpoints: `xahau.network` / `xahau-test.net` (NetworkID **21337** / **21338**)

### Intentional remaining opens

| Item | Notes |
|------|-------|
| **Import live** | Encode present; live submit needs an **xPOP** fixture |
| **XChain\*** | Out of scope; `src` encode stubs removed (catalog type codes only) |
| **DID / Credential / MPT / PermissionedDomain** | Amendments **absent** on public Xahau; `src` helpers removed (catalog only) |
| P2 pseudo decode | GenesisMint / EmitFailure / UNLReport if public blobs appear |
| Batch live | When BatchV1_1 is ON on a public net - `src` helper removed until then |
| Subscribe S3 | `books` order-book subscribe builders (DEX, deferred) |

---

## Shipped (0.3.1)

- [x] Xahau-only examples tree + EncodeOnly XRPL-port API cut from `src` (see release notes for 0.3.1)
- [x] Native host **123**/123 after cut

## Shipped (0.3.0)

- [x] Mainnet TLS fail-closed, Base58Check accounts, persistent WSS + RPC id match, API concurrency docs, multi-sign offline example
- [x] Xahau on-ledger maturity: see release notes for 0.3.0

## Shipped (0.2.2 / 0.2.1 / 0.2.0)

See git tags / release notes. Historical counts at 0.2.0 ship differ from the current **73** / **123** surface.

## Near term (post-0.3.1)

### Subscribe

- [x] S0-S2, S4 (stream MVP + WalletMonitor)
- [ ] S3: `books` order-book subscribe builders (DEX, deferred)

### Wallet-priority RPC builders

- [x] Account / ledger / book_offers / NFT helpers / feature / random (builders)
- [ ] Device RPC suite green on hardware (ongoing)
- [ ] Remaining public methods: path_find, ripple_path_find, mpt_holders, full ledger_entry types

### Release / polish

- [ ] Additional golden vectors as needed
- [ ] Arduino Library Manager listing (after public tag)
- [ ] Batch live smoke if/when BatchV1_1 is ON on a public net

## Product integrations (out of tree)

- [ ] Application / wallet products consume XahauClient as a library only  
  (no wallet UI or NVS keystore in this repository)

## Non-goals

- Admin RPC
- Full history / Clio replacement
- Smart-contract runtime (separate protocol work)
