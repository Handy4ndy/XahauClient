#!/usr/bin/env python3
"""
Reorganize examples/tx into Xahau official transaction feature groups.

Source of truth:
  https://Xahau.org/docs/references/protocol/transactions/types
  Groups: Accounts | Payments | Tokens | DEX | Decentralized Storage |
          Lending | Xahau Side Chains | Other

Payment is listed under both Accounts and Payments in the docs; we place it
under Payments (primary send path). NFT market offers live under Tokens with
mint/burn (docs also list them under DEX); classic Offer* + AMM* stay under DEX.
"""
from __future__ import annotations

import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TX = ROOT / "examples" / "tx"

# Folder name under examples/tx -> list of TxType example folder names
GROUPS: dict[str, list[str]] = {
    "Accounts": [
        "AccountSet",
        "AccountDelete",
        "SetRegularKey",
        "SignerListSet",
        "TicketCreate",
        "DepositPreauth",
        "DelegateSet",
    ],
    "Payments": [
        "Payment",
        "EscrowCreate",
        "EscrowFinish",
        "EscrowCancel",
        "CheckCreate",
        "CheckCash",
        "CheckCancel",
        "PaymentChannelCreate",
        "PaymentChannelFund",
        "PaymentChannelClaim",
    ],
    "Tokens": [
        "TrustSet",
        "Clawback",
        "MPTokenIssuanceCreate",
        "MPTokenIssuanceDestroy",
        "MPTokenIssuanceSet",
        "MPTokenAuthorize",
        "NFTokenMint",
        "NFTokenBurn",
        "NFTokenModify",
        "NFTokenCreateOffer",
        "NFTokenCancelOffer",
        "NFTokenAcceptOffer",
    ],
    "DEX": [
        "OfferCreate",
        "OfferCancel",
        "AMMCreate",
        "AMMDeposit",
        "AMMWithdraw",
        "AMMVote",
        "AMMBid",
        "AMMDelete",
        "AMMClawback",
    ],
    "DecentralizedStorage": [
        "DIDSet",
        "DIDDelete",
        "CredentialCreate",
        "CredentialAccept",
        "CredentialDelete",
        "OracleSet",
        "OracleDelete",
        "PermissionedDomainSet",
        "PermissionedDomainDelete",
    ],
    "Lending": [
        "VaultCreate",
        "VaultSet",
        "VaultDelete",
        "VaultDeposit",
        "VaultWithdraw",
        "VaultClawback",
        "LoanBrokerSet",
        "LoanBrokerDelete",
        "LoanBrokerCoverDeposit",
        "LoanBrokerCoverWithdraw",
        "LoanBrokerCoverClawback",
        "LoanSet",
        "LoanDelete",
        "LoanManage",
        "LoanPay",
    ],
    "SideChains": [
        "XChainCreateBridge",
        "XChainModifyBridge",
        "XChainCreateClaimID",
        "XChainCommit",
        "XChainClaim",
        "XChainAccountCreateCommit",
        "XChainAddClaimAttestation",
        "XChainAddAccountCreateAttestation",
    ],
    "Other": [
        "Batch",
    ],
}

GROUP_META = {
    "Accounts": {
        "title": "Accounts",
        "anchor": "accounts",
        "blurb": "Account options, keys, tickets, multi-sign, deposit auth, permission delegation.",
        "docs": "https://Xahau.org/docs/references/protocol/transactions/types#accounts",
    },
    "Payments": {
        "title": "Payments",
        "anchor": "payments",
        "blurb": "Payment, Escrow, Checks, Payment Channels.",
        "docs": "https://Xahau.org/docs/references/protocol/transactions/types#payments",
    },
    "Tokens": {
        "title": "Tokens",
        "anchor": "tokens",
        "blurb": "Trust lines, clawback, MPTs, NFTs (including NFT offers).",
        "docs": "https://Xahau.org/docs/references/protocol/transactions/types#tokens",
    },
    "DEX": {
        "title": "DEX",
        "anchor": "dex",
        "blurb": "Classic offers (order book) and Automated Market Makers (AMM).",
        "docs": "https://Xahau.org/docs/references/protocol/transactions/types#dex",
    },
    "DecentralizedStorage": {
        "title": "Decentralized Storage",
        "anchor": "decentralized-storage",
        "blurb": "DID, Credentials, Oracles, Permissioned Domains.",
        "docs": "https://Xahau.org/docs/references/protocol/transactions/types#decentralized-storage",
    },
    "Lending": {
        "title": "Lending",
        "anchor": "lending",
        "blurb": "Single-asset Vaults and Lending Protocol (brokers, loans). Prefer Devnet.",
        "docs": "https://Xahau.org/docs/references/protocol/transactions/types#lending",
    },
    "SideChains": {
        "title": "Xahau Side Chains",
        "anchor": "Xahau-side-chains",
        "blurb": "XChain bridge, commit, claim, and witness attestations.",
        "docs": "https://Xahau.org/docs/references/protocol/transactions/types#Xahau-side-chains",
    },
    "Other": {
        "title": "Other",
        "anchor": "other",
        "blurb": "Batch and miscellaneous user transactions (LedgerStateFix is catalog-only).",
        "docs": "https://Xahau.org/docs/references/protocol/transactions/types#other",
    },
}


def find_example_dir(name: str) -> Path | None:
    """Find Name/Name.ino whether flat or already under a group."""
    flat = TX / name
    if (flat / f"{name}.ino").is_file():
        return flat
    for g in GROUPS:
        nested = TX / g / name
        if (nested / f"{name}.ino").is_file():
            return nested
    return None


def main() -> None:
    TX.mkdir(parents=True, exist_ok=True)
    moved = []
    missing = []

    for group, names in GROUPS.items():
        gdir = TX / group
        gdir.mkdir(parents=True, exist_ok=True)
        for name in names:
            src = find_example_dir(name)
            if not src:
                missing.append(name)
                continue
            dest = gdir / name
            if src.resolve() == dest.resolve():
                continue
            if dest.exists():
                shutil.rmtree(dest)
            shutil.move(str(src), str(dest))
            moved.append(f"{src.relative_to(TX)} -> {group}/{name}")

    # Remove empty leftover dirs under tx (not group dirs, not README)
    for child in list(TX.iterdir()):
        if not child.is_dir():
            continue
        if child.name in GROUPS:
            continue
        # leftover empty or orphan
        if not any(child.rglob("*.ino")):
            shutil.rmtree(child, ignore_errors=True)

    write_readmes()
    print(f"Moved {len(moved)} examples")
    for m in moved[:20]:
        print(" ", m)
    if len(moved) > 20:
        print(f"  ... +{len(moved) - 20} more")
    if missing:
        print("MISSING:", missing)


def write_readmes() -> None:
    lines = [
        "# XahauClient transaction examples",
        "",
        "Grouped to match **rippled / Xahau docs** transaction feature areas:",
        "",
        "> [Transaction Types](https://Xahau.org/docs/references/protocol/transactions/types) -",
        "> Accounts | Payments | Tokens | DEX | Decentralized Storage | Lending |",
        "> Xahau Side Chains | Other",
        "",
        "## Layout",
        "",
        "```text",
        "examples/tx/",
    ]
    for g, names in GROUPS.items():
        lines.append(f"  {g}/          # {len(names)} sketches")
    lines += [
        "```",
        "",
        "Arduino IDE: **File -> Examples -> XahauClient -> tx -> &lt;Group&gt; -> &lt;TxName&gt;**",
        "",
        "## How to use",
        "",
        "1. Open a sketch under `examples/tx/<Group>/<Name>/`.",
        "2. Edit **CONFIG**: `WIFI_SSID`, `WIFI_PASSWORD`, `XAHAU_SEED`, `XAHAU_NETWORK`, `DO_SUBMIT`.",
        "3. Replace destinations / ledger IDs / amounts for your scenario.",
        "4. ESP32 + WiFi + [arduinoWebSockets](https://github.com/Links2004/arduinoWebSockets) for live submit.",
        "",
        "```cpp",
        'const char* XAHAU_SEED = "sEd...";   // same idea as Wallet.from_seed',
        "const int   XAHAU_NETWORK = 1;    // 0 mainnet | 1 testnet | 2 devnet",
        "```",
        "",
        "## Groups",
        "",
    ]

    for g, names in GROUPS.items():
        meta = GROUP_META[g]
        lines.append(f"### {meta['title']}")
        lines.append("")
        lines.append(f"{meta['blurb']}  ")
        lines.append(f"[Xahau docs -> {meta['title']}]({meta['docs']})")
        lines.append("")
        lines.append("| Example | Path |")
        lines.append("|---------|------|")
        for n in names:
            lines.append(f"| `{n}` | `tx/{g}/{n}/` |")
        lines.append("")

    lines += [
        "## Notes",
        "",
        "- **Payment** is documented under both Accounts and Payments; sketches live under **Payments**.",
        "- **NFT offers** are under **Tokens** with mint/burn (docs also list them under DEX).",
        "- **Vault + Loan** share the docs **Lending** section; default network in sketches is Devnet.",
        "- Pseudo-transactions (`EnableAmendment`, `SetFee`, `UNLModify`) have no examples.",
        "- `LedgerStateFix` is catalog-only (no library fill yet).",
        "",
        "## Shared helper",
        "",
        "`src/Xahau/sketch/example_runtime.h` - WiFi, network preset, sequence/fee, submit log, TLS notes.",
        "",
        "## Regenerate / reorganize",
        "",
        "```bash",
        "python protocol/scripts/generate_examples.py   # rebuild .ino bodies (flat or update script)",
        "python protocol/scripts/reorganize_examples.py # move into Xahau groups",
        "```",
        "",
        "## Security",
        "",
        "Never commit real seeds or WiFi passwords. Prefer Testnet/Devnet faucets.",
        "See [docs/SECURITY.md](../../docs/SECURITY.md).",
        "",
    ]
    (TX / "README.md").write_text("\n".join(lines) + "\n", encoding="utf-8")

    # Per-group short README
    for g, names in GROUPS.items():
        meta = GROUP_META[g]
        glines = [
            f"# {meta['title']}",
            "",
            meta["blurb"],
            "",
            f"Xahau docs: {meta['docs']}",
            "",
            "Sketches:",
            "",
        ]
        for n in names:
            glines.append(f"- [`{n}`](./{n}/{n}.ino)")
        glines.append("")
        (TX / g / "README.md").write_text("\n".join(glines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
