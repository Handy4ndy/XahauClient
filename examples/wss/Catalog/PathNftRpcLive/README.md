# PathNftRpcLive

Online WSS battery for **path / DEX / Clio-NFT** `rpcBuild*` helpers (needs Wi-Fi + seeds).

## Methods exercised

| Method | Builder | Xahau note |
|--------|---------|------------|
| deposit_authorized | `rpcBuildDepositAuthorized` | Primary useful check |
| book_offers | `rpcBuildBookOffers` | Empty books may server-error; transport Ok still counts |
| nft_info / nft_history / nft_* offers / nfts_by_issuer | `rpcBuildNft*` | Clio-compat; often server-error (NFToken disabled - prefer **URIToken**) |
| channel_verify | `rpcBuildChannelVerify` | Dummy values - server rejects; build+transport still exercised |

**Removed 2026-09-28:** AMM info JSON builder (`rpcBuildAmmInfo`) with the EncodeOnly cut. Do not restore unless AMM is live on both public nets.

## Success criteria

`build` + `rawRequest` -> `Status::Ok`. Server-side `"error"` is printed but does **not** fail the sketch.

Parent index: [../README.md](../README.md).
