# Remaining intentional gaps

Summary for **XahauClient 0.3.1**.

This lists what is **intentionally open** — not defects in the published ship surface.
Encode-only XRPL-port fill/sign APIs were removed from library `src` on 2026-09-28 (catalog type codes only). Details: [ENCODE_ONLY_STUBS.md](ENCODE_ONLY_STUBS.md), [LIBRARY_ROADMAP.md](LIBRARY_ROADMAP.md).

## Intentional opens

| Item | Status | Notes |
|------|--------|-------|
| **Import live** | Open until xPOP | Library encode for Import (type **97**) is present; live submit needs a valid **xPOP** fixture |
| **XChain\*** | Out of scope | No fill/sign wrappers in `src`; catalog type codes only |
| **DID / Credential / MPT / PermissionedDomain** | Amendments absent | Not enabled on public Xahau `feature` lists; `src` helpers removed; catalog codes only |
| **NFToken\* / AMM\* / Vault / Loan / Batch / DelegateSet** | Catalog only | Prefer **URIToken** for NFTs on Xahau. XRPL-port families are not part of the 0.3.1 ship surface |
| P2 pseudo decode | Optional | GenesisMint / EmitFailure / UNLReport binary decode if public blobs appear |
| Subscribe `books` | Deferred | Order-book subscribe builders (DEX phase S3) |

## What shipped (do not treat as gaps)

- Classic money path, Offers, Escrow, Checks, PayChan, Tickets, RegularKey, multi-sign
- Hooks: SetHook / Invoke (encode + live where proven)
- URIToken market (Mint / CreateSellOffer / Cancel / Buy / Burn)
- Remit (incl. MintURIToken / multi-IOU modes), ClaimReward, CronSet, SetRemarks, Oracle*
- Import **encode**; network presets Mainnet **21337** / Testnet **21338**
- Example pack **73** Xahau-only INOs; native host suite **123**/123

## How to verify on-network

```text
feature / server_definitions against:
  wss://xahau-test.net   (NetworkID 21338)
  wss://xahau.network    (NetworkID 21337)
```

Re-check after network upgrades before assuming a catalog-only family is live.
