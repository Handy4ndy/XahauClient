# Tokens

Trust lines, clawback, and **URIToken** (Xahau NFT model).

Xahau docs: https://docs.xahau.network/ (transaction types)

## Sketches (7) - Xahau mainnet/testnet

- [`TrustSet`](./TrustSet/TrustSet.ino)
- [`Clawback`](./Clawback/Clawback.ino)

### URIToken market (types 45-49)

- [`URITokenMint`](./URITokenMint/URITokenMint.ino) - mint (type 45)
- [`URITokenCreateSellOffer`](./URITokenCreateSellOffer/URITokenCreateSellOffer.ino) - list for sale (48)
- [`URITokenBuy`](./URITokenBuy/URITokenBuy.ino) - buy listed token (47)
- [`URITokenCancelSellOffer`](./URITokenCancelSellOffer/URITokenCancelSellOffer.ino) - cancel listing (49)
- [`URITokenBurn`](./URITokenBurn/URITokenBurn.ino) - burn owned token (46)

## Archived (not live on Xahau mainnet/testnet)

NFToken* and MPToken* example folders were moved to a maintainer archive. a maintainer archive (prefer URIToken;
MPT amendments absent on both nets). Library `src` EncodeOnly fill/sign helpers were **also removed** (catalog only; see ../../docs/ENCODE_ONLY_STUBS.md).
