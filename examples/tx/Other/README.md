# Other

Xahau-native user transactions (LedgerStateFix is catalog-only).

Xahau docs: https://docs.xahau.network/

## Sketches (6) - Xahau-native

- [`ClaimReward`](./ClaimReward/ClaimReward.ino) - code **98** (genesis / IOU / opt-out modes)
- [`SetRemarks`](./SetRemarks/SetRemarks.ino) - code **94** (remarks on owned object)
- [`CronSet`](./CronSet/CronSet.ino) - code **93** (schedule / unset via `CRON_MODE`)
- [`Invoke`](./Invoke/Invoke.ino) - code **99** (Hooks self-invoke; non-BIP)
- [`SetHook`](./SetHook/SetHook.ino) - code **22** (Hooks CreateCode; DO_SUBMIT=false default; MCU serial note)
- [`Import`](./Import/Import.ino) - code **97** (Xahau-native encode; live submit needs xPOP)

`Batch` (XRPL-port EncodeOnly) was archived - 
see a maintainer archive.
