# Payments

Payment, Checks, Escrow, Payment Channels, and Xahau-native Remit.

- [`Payment`](./Payment/Payment.ino) - single-sign XAH / IOU / MPT
- [`PaymentMultiSign`](./PaymentMultiSign/PaymentMultiSign.ino) - multi-sign Payment (Account A, Signer B)
- CheckCreate / CheckCash / CheckCancel
- EscrowCreate / EscrowFinish / EscrowCancel
- PaymentChannelCreate / Fund / Claim
- [`Remit`](./Remit/Remit.ino) - Xahau-native multi-asset push (code **95**)
 - `REMIT_MODE` 0=XAH, 1=+MintURIToken, 2=XAH+USD IOU, 3=URITokenIDs, 4=XAH+USD+URITokenIDs
