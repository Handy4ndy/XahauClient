#!/usr/bin/env python3
"""Generate XahauClient Arduino examples for every wired user-submittable tx."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TX = ROOT / "examples" / "tx"

# Xahau docs feature groups — keep sketches under examples/tx/<Group>/<Name>/
# See protocol/scripts/reorganize_examples.py and
# https://Xahau.org/docs/references/protocol/transactions/types
TX_GROUP = {
    "AccountSet": "Accounts",
    "AccountDelete": "Accounts",
    "SetRegularKey": "Accounts",
    "SignerListSet": "Accounts",
    "TicketCreate": "Accounts",
    "DepositPreauth": "Accounts",
    "DelegateSet": "Accounts",
    "Payment": "Payments",
    "EscrowCreate": "Payments",
    "EscrowFinish": "Payments",
    "EscrowCancel": "Payments",
    "CheckCreate": "Payments",
    "CheckCash": "Payments",
    "CheckCancel": "Payments",
    "PaymentChannelCreate": "Payments",
    "PaymentChannelFund": "Payments",
    "PaymentChannelClaim": "Payments",
    "TrustSet": "Tokens",
    "Clawback": "Tokens",
    "MPTokenIssuanceCreate": "Tokens",
    "MPTokenIssuanceDestroy": "Tokens",
    "MPTokenIssuanceSet": "Tokens",
    "MPTokenAuthorize": "Tokens",
    "NFTokenMint": "Tokens",
    "NFTokenBurn": "Tokens",
    "NFTokenModify": "Tokens",
    "NFTokenCreateOffer": "Tokens",
    "NFTokenCancelOffer": "Tokens",
    "NFTokenAcceptOffer": "Tokens",
    "OfferCreate": "DEX",
    "OfferCancel": "DEX",
    "AMMCreate": "DEX",
    "AMMDeposit": "DEX",
    "AMMWithdraw": "DEX",
    "AMMVote": "DEX",
    "AMMBid": "DEX",
    "AMMDelete": "DEX",
    "AMMClawback": "DEX",
    "DIDSet": "DecentralizedStorage",
    "DIDDelete": "DecentralizedStorage",
    "CredentialCreate": "DecentralizedStorage",
    "CredentialAccept": "DecentralizedStorage",
    "CredentialDelete": "DecentralizedStorage",
    "OracleSet": "DecentralizedStorage",
    "OracleDelete": "DecentralizedStorage",
    "PermissionedDomainSet": "DecentralizedStorage",
    "PermissionedDomainDelete": "DecentralizedStorage",
    "VaultCreate": "Lending",
    "VaultSet": "Lending",
    "VaultDelete": "Lending",
    "VaultDeposit": "Lending",
    "VaultWithdraw": "Lending",
    "VaultClawback": "Lending",
    "LoanBrokerSet": "Lending",
    "LoanBrokerDelete": "Lending",
    "LoanBrokerCoverDeposit": "Lending",
    "LoanBrokerCoverWithdraw": "Lending",
    "LoanBrokerCoverClawback": "Lending",
    "LoanSet": "Lending",
    "LoanDelete": "Lending",
    "LoanManage": "Lending",
    "LoanPay": "Lending",
    "XChainCreateBridge": "SideChains",
    "XChainModifyBridge": "SideChains",
    "XChainCreateClaimID": "SideChains",
    "XChainCommit": "SideChains",
    "XChainClaim": "SideChains",
    "XChainAccountCreateCommit": "SideChains",
    "XChainAddClaimAttestation": "SideChains",
    "XChainAddAccountCreateAttestation": "SideChains",
    "Batch": "Other",
}

HEADER = """/**
 * ============================================================================
 * XahauClient Example — {title}
 * ============================================================================
 *
 * Transaction type: {tx_type} (catalog code {code})
 *
 * What this sketch does
 * ---------------------
 * 1. Reads your classic family seed (like a JS/Python Xahau library: paste the seed string).
 * 2. Derives address + keys (Ed25519 if seed starts with sEd…, else secp256k1).
 * 3. Builds a {title} transaction using the library filler / builder.
 * 4. Optionally connects WiFi, opens a public cluster WebSocket, fetches Sequence/LLS/fee,
 *    signs, and submits.
 *
 * Live network guidance
 * ---------------------
 * {live_note}
 * Encode/sign always work offline; live success depends on amendments + funded account.
 *
 * Safety
 * ------
 * - NEVER commit real mainnet seeds or WiFi passwords.
 * - Prefer Testnet/Devnet faucets while learning.
 * - Replace DEST / IDs / amounts with your own values before submit.
 *
 * Board: ESP32 (WiFi + WebSocketsClient when XAHAU_FEATURE_NET=1)
 * Docs:  library docs/NETWORKS.md , docs/API.md
 */
"""

CONFIG = """
// ---------------------------------------------------------------------------
// CONFIG — edit like a .js / .py script (seed + WiFi only)
// ---------------------------------------------------------------------------
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Classic family seed: "sEd…" (Ed25519) or "s…" (secp256k1).
// Fund via Testnet/Devnet faucet, then paste here (same idea as Wallet.from_seed).
const char* XAHAU_SEED     = "sEdPLACEHOLDER_REPLACE_ME";

// 0 = Mainnet (real value!), 1 = Testnet (default), 2 = Devnet
const int   XAHAU_NETWORK  = {net};

// true = WiFi + submit; false = offline build/sign only (prints blob hex)
const bool  DO_SUBMIT     = true;

// Print full signed blob hex to Serial (can be long)
const bool  PRINT_BLOB    = true;
"""

INCLUDES = """
// Enable optional ESP32 WebSocket transport before the umbrella include.
#define XAHAU_FEATURE_NET 1
#include <WiFi.h>
#include <XahauClient.h>
#include <xahau/sketch/example_runtime.h>
#include <string.h>
// ESP32 loopTask default stack is ~8 KiB; raise it for WiFi + WSS + crypto.
#if defined(ESP_PLATFORM) && defined(SET_LOOP_TASK_STACK_SIZE)
SET_LOOP_TASK_STACK_SIZE(16 * 1024);
#endif
"""

SETUP_HEAD = """
void setup() {{
  Serial.begin(115200);
  delay(1500);
  Serial.println();
  Serial.println(F("=== XahauClient · {title} ==="));
  Serial.print(F("Library "));
  Serial.println(XAHAU_CLIENT_VERSION);

  // ---- keys from seed (like Xahau-py / Xahau.js Wallet.from_seed) ----
  xahau::KeyPair keys;
  xahau::Status st = xahau::deriveKeysFromSeed(XAHAU_SEED, keys);
  if (st != xahau::Status::Ok) {{
    Serial.print(F("deriveKeysFromSeed failed: "));
    Serial.println(xahau::statusName(st));
    Serial.println(F("Paste a valid family seed into XAHAU_SEED."));
    return;
  }}
  Serial.print(F("Address: "));
  Serial.println(keys.address);
  Serial.print(F("Curve:   "));
  Serial.println(keys.curve == xahau::Curve::Ed25519 ? F("Ed25519") : F("secp256k1"));

  // ---- build typed params ----
"""

FILL_SIGN_BLOCK = """
  // BSS (not stack) — ESP32 task stack is ~8 KiB; bags + WebSockets need room.
  static char s_blob[2048];
  static xahau::TxFieldBag s_bag;
  auto doSign = [&]() -> xahau::Status {{
    s_bag.clear();
    xahau::Status s = xahau::fill{name}(p, keys.accountId, keys.publicKey, s_bag);
    if (s != xahau::Status::Ok) return s;
    return xahau::signTx(s_bag, keys, s_blob, sizeof(s_blob));
  }};
"""

BUILD_SIGN_BLOCK = """
  static char s_blob[2048];
  auto doSign = [&]() -> xahau::Status {{
    return xahau::buildAndSign{name}(keys, p, s_blob, sizeof(s_blob));
  }};
"""

SETUP_TAIL = """
  // ---- sign helper (rebuild after network seq/fee refresh) ----
{sign_block}
  st = doSign();
  Serial.print(F("sign: "));
  Serial.println(xahau::statusName(st));
  if (st != xahau::Status::Ok) {{
    xahau::secureClear(keys);
    return;
  }}
  if (PRINT_BLOB) {{
    Serial.println(F("Signed blob (hex):"));
    Serial.println(s_blob);
  }}

  if (!DO_SUBMIT) {{
    Serial.println(F("DO_SUBMIT=false - offline only. Done."));
    xahau::secureClear(keys);
    return;
  }}

#if XAHAU_FEATURE_NET
  if (!xahau::sketch::wifiConnect(WIFI_SSID, WIFI_PASSWORD)) {{
    xahau::secureClear(keys);
    return;
  }}

  const xahau::NetworkEndpoint& net = xahau::sketch::networkFromIndex(XAHAU_NETWORK);
  Serial.print(F("Network: "));
  Serial.print(net.name);
  Serial.print(F("  wss://"));
  Serial.print(net.wsHost);
  Serial.print(F(":"));
  Serial.println(net.wsPort);

  // CRITICAL: WebSocketsClient is large — never put WsXahauTransport on setup() stack.
  static xahau::WsXahauTransport s_ws;
  static xahau::XahauRpcClient s_client(s_ws);
  s_ws.setEndpoint(net);

  st = xahau::sketch::prepareCommon(s_client, keys, {common});
  if (st != xahau::Status::Ok) {{
    Serial.print(F("prepareCommon failed: "));
    Serial.println(xahau::statusName(st));
    Serial.println(F("Is the account funded on this network?"));
    xahau::secureClear(keys);
    return;
  }}
  Serial.print(F("Sequence: "));
  Serial.println({common}.sequence);
  Serial.print(F("LLS:      "));
  Serial.println({common}.lastLedgerSequence);
  Serial.print(F("Fee:      "));
  Serial.println((unsigned long){common}.feeDrops);

  st = doSign();
  if (st != xahau::Status::Ok) {{
    Serial.print(F("re-sign failed: "));
    Serial.println(xahau::statusName(st));
    xahau::secureClear(keys);
    return;
  }}
  xahau::sketch::submitAndLog(s_client, s_blob);
#else
  Serial.println(F("XAHAU_FEATURE_NET disabled - cannot submit."));
#endif

  xahau::secureClear(keys);
  Serial.println(F("Done."));
}}

void loop() {{}}
"""

DEST = "rYOUR_DESTINATION_ADDRESS"  # replace with your destination r-address
HASH32 = "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
ISS24 = "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
# Valid-shape classic address placeholder (replace with real issuer / counterparty).
ISSUER = DEST


def common_fee(path="p.common"):
    return f"""  {path}.feeDrops = 12;
  {path}.sequence = 1;              // overwritten when DO_SUBMIT
  {path}.lastLedgerSequence = 100;  // overwritten when DO_SUBMIT
  xahau::sketch::applyCanonicalFlag({path}, keys.curve);
"""


def add(examples, folder, title, code, net, live, params, builder, use_build=True, common="p.common"):
    examples.append(
        dict(
            folder=folder,
            title=title,
            code=code,
            net=net,
            live=live,
            params=params,
            builder=builder,
            use_build=use_build,
            common=common,
        )
    )


STRUCT = {
    "Payment": "PaymentParams",
    "AccountSet": "AccountSetParams",
    "TrustSet": "TrustSetParams",
    "OfferCreate": "OfferCreateParams",
    "OfferCancel": "OfferCancelParams",
    "TicketCreate": "TicketCreateParams",
    "SetRegularKey": "SetRegularKeyParams",
    "SignerListSet": "SignerListSetParams",
    "EscrowCreate": "EscrowCreateParams",
    "EscrowFinish": "EscrowFinishParams",
    "EscrowCancel": "EscrowCancelParams",
    "CheckCreate": "CheckCreateParams",
    "CheckCash": "CheckCashParams",
    "CheckCancel": "CheckCancelParams",
    "PaymentChannelCreate": "PaymentChannelCreateParams",
    "PaymentChannelFund": "PaymentChannelFundParams",
    "PaymentChannelClaim": "PaymentChannelClaimParams",
    "NFTokenMint": "NFTokenMintParams",
    "NFTokenBurn": "NFTokenBurnParams",
    "NFTokenCreateOffer": "NFTokenCreateOfferParams",
    "NFTokenCancelOffer": "NFTokenCancelOfferParams",
    "NFTokenAcceptOffer": "NFTokenAcceptOfferParams",
    "NFTokenModify": "NFTokenModifyParams",
    "DepositPreauth": "DepositPreauthParams",
    "AccountDelete": "AccountDeleteParams",
    "Clawback": "ClawbackParams",
    "DIDSet": "DIDSetParams",
    "DIDDelete": "DIDDeleteParams",
    "CredentialCreate": "CredentialCreateParams",
    "CredentialAccept": "CredentialAcceptParams",
    "CredentialDelete": "CredentialDeleteParams",
    "OracleSet": "OracleSetParams",
    "OracleDelete": "OracleDeleteParams",
    "AMMCreate": "AMMCreateParams",
    "AMMDeposit": "AMMAssetPairParams",
    "AMMWithdraw": "AMMAssetPairParams",
    "AMMVote": "AMMAssetPairParams",
    "AMMBid": "AMMAssetPairParams",
    "AMMDelete": "AMMAssetPairParams",
    "AMMClawback": "AMMClawbackParams",
    "MPTokenIssuanceCreate": "MPTokenIssuanceCreateParams",
    "MPTokenIssuanceDestroy": "MPTokenIssuanceIdParams",
    "MPTokenIssuanceSet": "MPTokenIssuanceIdParams",
    "MPTokenAuthorize": "MPTokenIssuanceIdParams",
    "PermissionedDomainSet": "PermissionedDomainSetParams",
    "PermissionedDomainDelete": "PermissionedDomainDeleteParams",
    "DelegateSet": "DelegateSetParams",
    "Batch": "BatchParams",
    "VaultCreate": "VaultCreateParams",
    "VaultSet": "VaultIdParams",
    "VaultDelete": "VaultIdParams",
    "VaultDeposit": "VaultAmountParams",
    "VaultWithdraw": "VaultAmountParams",
    "VaultClawback": "VaultAmountParams",
    "XChainCreateBridge": "XChainBridgeParams",
    "XChainModifyBridge": "XChainBridgeParams",
    "XChainCreateClaimID": "XChainBridgeParams",
    "XChainCommit": "XChainBridgeParams",
    "XChainClaim": "XChainBridgeParams",
    "XChainAccountCreateCommit": "XChainBridgeParams",
    "XChainAddClaimAttestation": "XChainAddClaimAttestationParams",
    "XChainAddAccountCreateAttestation": "XChainAddAccountCreateAttestationParams",
    "LoanBrokerSet": "LoanBrokerIdParams",
    "LoanBrokerDelete": "LoanBrokerIdParams",
    "LoanBrokerCoverDeposit": "LoanBrokerCoverParams",
    "LoanBrokerCoverWithdraw": "LoanBrokerCoverParams",
    "LoanBrokerCoverClawback": "LoanBrokerCoverParams",
    "LoanSet": "LoanParams",
    "LoanDelete": "LoanParams",
    "LoanManage": "LoanParams",
    "LoanPay": "LoanParams",
}


def build_examples():
    E = []
    add(
        E,
        "Payment",
        "Payment",
        0,
        1,
        "AllPublic — Testnet or Mainnet. Classic XRP/IOU payment.",
        f"""  strncpy(p.destination, "{DEST}", sizeof(p.destination) - 1);
  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 1000000ULL;  // 1 XRP
  // strncpy(p.memoData, "hello from ESP32", sizeof(p.memoData) - 1);
"""
        + common_fee(),
        "Payment",
        True,
    )
    add(
        E,
        "AccountSet",
        "AccountSet",
        3,
        1,
        "AllPublic — set account flags/domain (example: asfDefaultRipple = 8).",
        """  p.hasSetFlag = true;
  p.setFlag = 8;  // asfDefaultRipple — change as needed
"""
        + common_fee(),
        "AccountSet",
        True,
    )
    add(
        E,
        "TrustSet",
        "TrustSet",
        20,
        1,
        "AllPublic — create/adjust trust line to an IOU issuer.",
        f"""  strncpy(p.limit.value, "1000", sizeof(p.limit.value) - 1);
  strncpy(p.limit.currency, "USD", sizeof(p.limit.currency) - 1);
  strncpy(p.limit.issuer, "{ISSUER}", sizeof(p.limit.issuer) - 1);
"""
        + common_fee(),
        "TrustSet",
        True,
    )
    add(
        E,
        "OfferCreate",
        "OfferCreate",
        7,
        1,
        "AllPublic — DEX order (TakerPays / TakerGets).",
        f"""  p.takerPays.kind = xahau::AmountKind::XrpDrops;
  p.takerPays.drops = 1000000ULL;
  p.takerGets.kind = xahau::AmountKind::Issued;
  strncpy(p.takerGets.issued.value, "1", sizeof(p.takerGets.issued.value) - 1);
  strncpy(p.takerGets.issued.currency, "USD", sizeof(p.takerGets.issued.currency) - 1);
  strncpy(p.takerGets.issued.issuer, "{ISSUER}", sizeof(p.takerGets.issued.issuer) - 1);
"""
        + common_fee(),
        "OfferCreate",
        True,
    )
    add(
        E,
        "OfferCancel",
        "OfferCancel",
        8,
        1,
        "AllPublic — cancel offer by sequence.",
        """  p.offerSequence = 1;
"""
        + common_fee(),
        "OfferCancel",
        True,
    )
    add(
        E,
        "TicketCreate",
        "TicketCreate",
        10,
        1,
        "AllPublic — reserve ticket sequences.",
        """  p.ticketCount = 1;
"""
        + common_fee(),
        "TicketCreate",
        True,
    )
    add(
        E,
        "SetRegularKey",
        "SetRegularKey",
        5,
        1,
        "AllPublic — set or clear a regular key.",
        f"""  strncpy(p.regularKey, "{DEST}", sizeof(p.regularKey) - 1);
"""
        + common_fee(),
        "SetRegularKey",
        True,
    )
    add(
        E,
        "SignerListSet",
        "SignerListSet",
        12,
        1,
        "AllPublic — multi-sign list (quorum 0 + empty = delete list demo).",
        """  p.signerQuorum = 0;
  p.entryCount = 0;
"""
        + common_fee(),
        "SignerListSet",
        True,
    )
    add(
        E,
        "EscrowCreate",
        "EscrowCreate",
        1,
        1,
        "AllPublic — lock XRP until FinishAfter / CancelAfter.",
        f"""  strncpy(p.destination, "{DEST}", sizeof(p.destination) - 1);
  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 1000000ULL;
  p.finishAfter = 0;
  p.cancelAfter = 0;
"""
        + common_fee(),
        "EscrowCreate",
        True,
    )
    add(
        E,
        "EscrowFinish",
        "EscrowFinish",
        2,
        1,
        "AllPublic — finish an escrow (Owner + OfferSequence).",
        f"""  strncpy(p.owner, "{DEST}", sizeof(p.owner) - 1);
  p.offerSequence = 1;
"""
        + common_fee(),
        "EscrowFinish",
        True,
    )
    add(
        E,
        "EscrowCancel",
        "EscrowCancel",
        4,
        1,
        "AllPublic — cancel expired escrow.",
        f"""  strncpy(p.owner, "{DEST}", sizeof(p.owner) - 1);
  p.offerSequence = 1;
"""
        + common_fee(),
        "EscrowCancel",
        True,
    )
    add(
        E,
        "CheckCreate",
        "CheckCreate",
        16,
        1,
        "AllPublic — create a Check (SendMax).",
        f"""  strncpy(p.destination, "{DEST}", sizeof(p.destination) - 1);
  p.sendMax.kind = xahau::AmountKind::XrpDrops;
  p.sendMax.drops = 1000000ULL;
"""
        + common_fee(),
        "CheckCreate",
        True,
    )
    add(
        E,
        "CheckCash",
        "CheckCash",
        17,
        1,
        "AllPublic — cash a Check by ID.",
        f"""  if (!xahau::sketch::parseHash256("{HASH32}", p.checkId)) {{
    Serial.println(F("Bad CheckID hex — replace with real ID"));
    return;
  }}
  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 1000000ULL;
"""
        + common_fee(),
        "CheckCash",
        True,
    )
    add(
        E,
        "CheckCancel",
        "CheckCancel",
        18,
        1,
        "AllPublic — cancel a Check by ID.",
        f"""  if (!xahau::sketch::parseHash256("{HASH32}", p.checkId)) {{
    Serial.println(F("Bad CheckID hex"));
    return;
  }}
"""
        + common_fee(),
        "CheckCancel",
        True,
    )
    add(
        E,
        "PaymentChannelCreate",
        "PaymentChannelCreate",
        13,
        1,
        "AllPublic — open payment channel.",
        f"""  strncpy(p.destination, "{DEST}", sizeof(p.destination) - 1);
  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 10000000ULL;
  p.settleDelay = 86400;
  memcpy(p.publicKey, keys.publicKey, 33);
  p.publicKeyLen = 33;
"""
        + common_fee(),
        "PaymentChannelCreate",
        True,
    )
    add(
        E,
        "PaymentChannelFund",
        "PaymentChannelFund",
        14,
        1,
        "AllPublic — add XRP to a channel.",
        f"""  if (!xahau::sketch::parseHash256("{HASH32}", p.channel)) {{
    Serial.println(F("Bad Channel ID hex"));
    return;
  }}
  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 1000000ULL;
"""
        + common_fee(),
        "PaymentChannelFund",
        True,
    )
    add(
        E,
        "PaymentChannelClaim",
        "PaymentChannelClaim",
        15,
        1,
        "AllPublic — claim from channel (add Balance/Signature for real claims).",
        f"""  if (!xahau::sketch::parseHash256("{HASH32}", p.channel)) {{
    Serial.println(F("Bad Channel ID hex"));
    return;
  }}
"""
        + common_fee(),
        "PaymentChannelClaim",
        True,
    )
    add(
        E,
        "NFTokenMint",
        "NFTokenMint",
        25,
        1,
        "AllPublic — mint an NFToken.",
        """  p.nfTokenTaxon = 0;
  strncpy(p.uri, "https://example.com/nft/1", sizeof(p.uri) - 1);
"""
        + common_fee(),
        "NFTokenMint",
        True,
    )
    add(
        E,
        "NFTokenBurn",
        "NFTokenBurn",
        26,
        1,
        "AllPublic — burn an NFToken by ID.",
        f"""  if (!xahau::sketch::parseHash256("{HASH32}", p.nfTokenId)) {{
    Serial.println(F("Bad NFTokenID hex"));
    return;
  }}
"""
        + common_fee(),
        "NFTokenBurn",
        True,
    )
    add(
        E,
        "NFTokenCreateOffer",
        "NFTokenCreateOffer",
        27,
        1,
        "AllPublic — sell/buy offer for an NFToken.",
        f"""  if (!xahau::sketch::parseHash256("{HASH32}", p.nfTokenId)) {{
    Serial.println(F("Bad NFTokenID hex"));
    return;
  }}
  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 1000000ULL;
"""
        + common_fee(),
        "NFTokenCreateOffer",
        True,
    )
    add(
        E,
        "NFTokenCancelOffer",
        "NFTokenCancelOffer",
        28,
        1,
        "AllPublic — cancel NFToken offer(s).",
        f"""  if (!xahau::sketch::parseHash256("{HASH32}", p.offerIds[0])) {{
    Serial.println(F("Bad OfferID hex"));
    return;
  }}
  p.offerCount = 1;
"""
        + common_fee(),
        "NFTokenCancelOffer",
        True,
    )
    add(
        E,
        "NFTokenAcceptOffer",
        "NFTokenAcceptOffer",
        29,
        1,
        "AllPublic — accept sell and/or buy offer.",
        f"""  p.hasSellOffer = true;
  if (!xahau::sketch::parseHash256("{HASH32}", p.sellOffer)) {{
    Serial.println(F("Bad SellOffer ID hex"));
    return;
  }}
"""
        + common_fee(),
        "NFTokenAcceptOffer",
        True,
    )
    add(
        E,
        "NFTokenModify",
        "NFTokenModify",
        61,
        1,
        "AllPublic (amendment) — modify NFToken URI.",
        f"""  if (!xahau::sketch::parseHash256("{HASH32}", p.nfTokenId)) {{
    Serial.println(F("Bad NFTokenID hex"));
    return;
  }}
  strncpy(p.uri, "https://example.com/nft/1-updated", sizeof(p.uri) - 1);
"""
        + common_fee(),
        "NFTokenModify",
        True,
    )
    add(
        E,
        "DepositPreauth",
        "DepositPreauth",
        19,
        1,
        "AllPublic — preauthorize an account for deposits.",
        f"""  strncpy(p.authorize, "{DEST}", sizeof(p.authorize) - 1);
"""
        + common_fee(),
        "DepositPreauth",
        True,
    )
    add(
        E,
        "AccountDelete",
        "AccountDelete",
        21,
        1,
        "AllPublic — delete account (strict requirements).",
        f"""  strncpy(p.destination, "{DEST}", sizeof(p.destination) - 1);
  Serial.println(F("WARNING: AccountDelete is irreversible when successful."));
"""
        + common_fee(),
        "AccountDelete",
        True,
    )
    add(
        E,
        "Clawback",
        "Clawback",
        30,
        1,
        "StableParallel — issuer claws back issued currency.",
        f"""  p.amount.kind = xahau::AmountKind::Issued;
  strncpy(p.amount.issued.value, "1", sizeof(p.amount.issued.value) - 1);
  strncpy(p.amount.issued.currency, "USD", sizeof(p.amount.issued.currency) - 1);
  strncpy(p.amount.issued.issuer, keys.address, sizeof(p.amount.issued.issuer) - 1);
"""
        + common_fee(),
        "Clawback",
        True,
    )
    # Phase4 fill-only
    for folder, code, live, params, use_b in [
        (
            "DIDSet",
            49,
            "DevnetFirst — set DID Document fields.",
            """  strncpy(p.uri, "did:example:esp32", sizeof(p.uri) - 1);
  strncpy(p.data, "{}", sizeof(p.data) - 1);
"""
            + common_fee(),
            False,
        ),
        ("DIDDelete", 50, "DevnetFirst — delete DID object.", common_fee(), False),
        (
            "CredentialCreate",
            58,
            "DevnetFirst — issue credential to Subject.",
            f"""  strncpy(p.subject, "{DEST}", sizeof(p.subject) - 1);
  strncpy(p.credentialType, "KYC", sizeof(p.credentialType) - 1);
"""
            + common_fee(),
            False,
        ),
        (
            "CredentialAccept",
            59,
            "DevnetFirst — subject accepts credential.",
            f"""  strncpy(p.issuer, "{ISSUER}", sizeof(p.issuer) - 1);
  strncpy(p.credentialType, "KYC", sizeof(p.credentialType) - 1);
"""
            + common_fee(),
            False,
        ),
        (
            "CredentialDelete",
            60,
            "DevnetFirst — delete credential.",
            f"""  strncpy(p.issuer, "{ISSUER}", sizeof(p.issuer) - 1);
  strncpy(p.subject, "{DEST}", sizeof(p.subject) - 1);
  strncpy(p.credentialType, "KYC", sizeof(p.credentialType) - 1);
"""
            + common_fee(),
            False,
        ),
        (
            "OracleSet",
            51,
            "DevnetFirst — publish oracle price data.",
            """  p.oracleDocumentID = 1;
  p.lastUpdateTime = 1700000000UL;
  strncpy(p.baseAsset, "XRP", sizeof(p.baseAsset) - 1);
  strncpy(p.quoteAsset, "USD", sizeof(p.quoteAsset) - 1);
  p.hasPrice = true;
  p.assetPrice = 500000ULL;
  p.scale = 6;
"""
            + common_fee(),
            False,
        ),
    ]:
        add(E, folder, folder, code, 2, live, params, folder, use_b)

    add(
        E,
        "OracleDelete",
        "OracleDelete",
        52,
        2,
        "DevnetFirst — delete oracle document.",
        """  p.oracleDocumentID = 1;
"""
        + common_fee(),
        "OracleDelete",
        True,
    )

    add(
        E,
        "AMMCreate",
        "AMMCreate",
        35,
        1,
        "StableParallel — create AMM pool.",
        f"""  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 10000000ULL;
  p.amount2.kind = xahau::AmountKind::Issued;
  strncpy(p.amount2.issued.value, "10", sizeof(p.amount2.issued.value) - 1);
  strncpy(p.amount2.issued.currency, "USD", sizeof(p.amount2.issued.currency) - 1);
  strncpy(p.amount2.issued.issuer, "{ISSUER}", sizeof(p.amount2.issued.issuer) - 1);
  p.tradingFee = 500;
"""
        + common_fee(),
        "AMMCreate",
        False,
    )
    amm = f"""  p.assetIsXrp = true;
  p.asset2IsXrp = false;
  strncpy(p.asset2Currency, "USD", sizeof(p.asset2Currency) - 1);
  strncpy(p.asset2Issuer, "{ISSUER}", sizeof(p.asset2Issuer) - 1);
"""
    add(
        E,
        "AMMDeposit",
        "AMMDeposit",
        36,
        1,
        "StableParallel — deposit into AMM.",
        amm
        + """  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 1000000ULL;
"""
        + common_fee(),
        "AMMDeposit",
        False,
    )
    add(
        E,
        "AMMWithdraw",
        "AMMWithdraw",
        37,
        1,
        "StableParallel — withdraw from AMM.",
        amm
        + """  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 1000000ULL;
"""
        + common_fee(),
        "AMMWithdraw",
        False,
    )
    add(
        E,
        "AMMVote",
        "AMMVote",
        38,
        1,
        "StableParallel — vote trading fee.",
        amm + """  p.tradingFee = 500;\n""" + common_fee(),
        "AMMVote",
        False,
    )
    add(E, "AMMBid", "AMMBid", 39, 1, "StableParallel — bid for auction slot.", amm + common_fee(), "AMMBid", False)
    add(E, "AMMDelete", "AMMDelete", 40, 1, "StableParallel — delete empty AMM.", amm + common_fee(), "AMMDelete", False)
    add(
        E,
        "AMMClawback",
        "AMMClawback",
        31,
        1,
        "StableParallel — clawback from AMM.",
        f"""  strncpy(p.assetCurrency, "USD", sizeof(p.assetCurrency) - 1);
  strncpy(p.assetIssuer, keys.address, sizeof(p.assetIssuer) - 1);
  strncpy(p.holder, "{DEST}", sizeof(p.holder) - 1);
  p.amount.kind = xahau::AmountKind::Issued;
  strncpy(p.amount.issued.value, "1", sizeof(p.amount.issued.value) - 1);
  strncpy(p.amount.issued.currency, "USD", sizeof(p.amount.issued.currency) - 1);
  strncpy(p.amount.issued.issuer, keys.address, sizeof(p.amount.issued.issuer) - 1);
"""
        + common_fee(),
        "AMMClawback",
        False,
    )
    add(
        E,
        "MPTokenIssuanceCreate",
        "MPTokenIssuanceCreate",
        54,
        2,
        "DevnetFirst — create MPT issuance.",
        """  p.assetScale = 2;
  p.maximumAmount = 1000000ULL;
"""
        + common_fee(),
        "MPTokenIssuanceCreate",
        False,
    )
    for folder, code, note in [
        ("MPTokenIssuanceDestroy", 55, "destroy MPT issuance"),
        ("MPTokenIssuanceSet", 56, "update MPT issuance"),
        ("MPTokenAuthorize", 57, "authorize holder for MPT"),
    ]:
        add(
            E,
            folder,
            folder,
            code,
            2,
            f"DevnetFirst — {note}.",
            f"""  if (!xahau::sketch::parseIssuanceId("{ISS24}", p.issuanceId)) {{
    Serial.println(F("Bad MPTokenIssuanceID hex (48 chars)"));
    return;
  }}
"""
            + common_fee(),
            folder,
            False,
        )
    add(
        E,
        "PermissionedDomainSet",
        "PermissionedDomainSet",
        62,
        2,
        "DevnetFirst — set permissioned domain.",
        f"""  memset(p.domainId, 0x11, sizeof(p.domainId));
  strncpy(p.credentialType, "KYC", sizeof(p.credentialType) - 1);
  strncpy(p.credentialIssuer, "{ISSUER}", sizeof(p.credentialIssuer) - 1);
"""
        + common_fee(),
        "PermissionedDomainSet",
        False,
    )
    add(
        E,
        "PermissionedDomainDelete",
        "PermissionedDomainDelete",
        63,
        2,
        "DevnetFirst — delete permissioned domain.",
        """  memset(p.domainId, 0x11, sizeof(p.domainId));
"""
        + common_fee(),
        "PermissionedDomainDelete",
        False,
    )
    add(
        E,
        "DelegateSet",
        "DelegateSet",
        64,
        2,
        "DevnetFirst — delegate permissions.",
        f"""  strncpy(p.authorize, "{DEST}", sizeof(p.authorize) - 1);
"""
        + common_fee(),
        "DelegateSet",
        False,
    )
    add(
        E,
        "Batch",
        "Batch",
        71,
        2,
        "DevnetFirst — batch envelope (skeleton; extend RawTransactions for production).",
        common_fee()
        + """  Serial.println(F("Note: Batch fill is a skeleton example."));
""",
        "Batch",
        False,
    )
    add(
        E,
        "VaultCreate",
        "VaultCreate",
        65,
        2,
        "DevnetFirst — create vault (prefer Devnet).",
        f"""  strncpy(p.assetCurrency, "USD", sizeof(p.assetCurrency) - 1);
  strncpy(p.assetIssuer, "{ISSUER}", sizeof(p.assetIssuer) - 1);
  p.assetsMaximum = 1000000ULL;
"""
        + common_fee(),
        "VaultCreate",
        False,
    )
    for folder, code, note in [
        ("VaultSet", 66, "update vault"),
        ("VaultDelete", 67, "delete vault"),
    ]:
        add(
            E,
            folder,
            folder,
            code,
            2,
            f"DevnetFirst — {note}.",
            f"""  if (!xahau::sketch::parseHash256("{HASH32}", p.vaultId)) {{
    Serial.println(F("Bad VaultID hex"));
    return;
  }}
"""
            + common_fee(),
            folder,
            False,
        )
    add(
        E,
        "VaultDeposit",
        "VaultDeposit",
        68,
        2,
        "DevnetFirst — deposit into vault.",
        f"""  if (!xahau::sketch::parseHash256("{HASH32}", p.vaultId)) {{
    Serial.println(F("Bad VaultID hex"));
    return;
  }}
  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 1000000ULL;
"""
        + common_fee(),
        "VaultDeposit",
        False,
    )
    add(
        E,
        "VaultWithdraw",
        "VaultWithdraw",
        69,
        2,
        "DevnetFirst — withdraw from vault.",
        f"""  if (!xahau::sketch::parseHash256("{HASH32}", p.vaultId)) {{
    Serial.println(F("Bad VaultID hex"));
    return;
  }}
  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 1000000ULL;
  strncpy(p.destination, "{DEST}", sizeof(p.destination) - 1);
"""
        + common_fee(),
        "VaultWithdraw",
        False,
    )
    add(
        E,
        "VaultClawback",
        "VaultClawback",
        70,
        2,
        "DevnetFirst — clawback from vault holder.",
        f"""  if (!xahau::sketch::parseHash256("{HASH32}", p.vaultId)) {{
    Serial.println(F("Bad VaultID hex"));
    return;
  }}
  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 1000000ULL;
  strncpy(p.holder, "{DEST}", sizeof(p.holder) - 1);
"""
        + common_fee(),
        "VaultClawback",
        False,
    )

    xbridge = f"""  strncpy(p.lockingDoor, "{DEST}", sizeof(p.lockingDoor) - 1);
  strncpy(p.issuingDoor, "{ISSUER}", sizeof(p.issuingDoor) - 1);
  p.signatureReward.kind = xahau::AmountKind::XrpDrops;
  p.signatureReward.drops = 100ULL;
"""
    add(
        E,
        "XChainCreateBridge",
        "XChainCreateBridge",
        48,
        1,
        "StableParallel — create cross-chain bridge.",
        xbridge
        + """  p.minAccountCreateAmount.kind = xahau::AmountKind::XrpDrops;
  p.minAccountCreateAmount.drops = 1000000ULL;
"""
        + common_fee(),
        "XChainCreateBridge",
        False,
    )
    add(
        E,
        "XChainModifyBridge",
        "XChainModifyBridge",
        47,
        1,
        "StableParallel — modify bridge.",
        xbridge + common_fee(),
        "XChainModifyBridge",
        False,
    )
    add(
        E,
        "XChainCreateClaimID",
        "XChainCreateClaimID",
        41,
        1,
        "StableParallel — create claim ID.",
        xbridge + common_fee(),
        "XChainCreateClaimID",
        False,
    )
    add(
        E,
        "XChainCommit",
        "XChainCommit",
        42,
        1,
        "StableParallel — commit for cross-chain claim.",
        xbridge
        + """  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 1000000ULL;
  p.xChainClaimID = 1;
"""
        + common_fee(),
        "XChainCommit",
        False,
    )
    add(
        E,
        "XChainClaim",
        "XChainClaim",
        43,
        1,
        "StableParallel — claim on destination chain.",
        xbridge
        + f"""  p.xChainClaimID = 1;
  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 1000000ULL;
  strncpy(p.destination, "{DEST}", sizeof(p.destination) - 1);
"""
        + common_fee(),
        "XChainClaim",
        False,
    )
    add(
        E,
        "XChainAccountCreateCommit",
        "XChainAccountCreateCommit",
        44,
        1,
        "StableParallel — account-create commit.",
        xbridge
        + f"""  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 10000000ULL;
  strncpy(p.destination, "{DEST}", sizeof(p.destination) - 1);
"""
        + common_fee(),
        "XChainAccountCreateCommit",
        False,
    )
    add(
        E,
        "XChainAddClaimAttestation",
        "XChainAddClaimAttestation",
        45,
        1,
        "StableParallel — witness claim attestation (needs real witness signature).",
        f"""  strncpy(p.base.lockingDoor, "{DEST}", sizeof(p.base.lockingDoor) - 1);
  strncpy(p.base.issuingDoor, "{ISSUER}", sizeof(p.base.issuingDoor) - 1);
  memcpy(p.base.publicKey, keys.publicKey, 33);
  p.base.publicKeyLen = 33;
  strncpy(p.base.otherChainSource, "{DEST}", sizeof(p.base.otherChainSource) - 1);
  strncpy(p.base.attestationRewardAccount, keys.address, sizeof(p.base.attestationRewardAccount) - 1);
  strncpy(p.base.attestationSignerAccount, keys.address, sizeof(p.base.attestationSignerAccount) - 1);
  p.base.wasLockingChainSend = 1;
  p.base.amount.kind = xahau::AmountKind::XrpDrops;
  p.base.amount.drops = 1000000ULL;
  p.xChainClaimID = 1;
"""
        + common_fee("p.base.common"),
        "XChainAddClaimAttestation",
        True,
        "p.base.common",
    )
    add(
        E,
        "XChainAddAccountCreateAttestation",
        "XChainAddAccountCreateAttestation",
        46,
        1,
        "StableParallel — witness account-create attestation.",
        f"""  strncpy(p.base.lockingDoor, "{DEST}", sizeof(p.base.lockingDoor) - 1);
  strncpy(p.base.issuingDoor, "{ISSUER}", sizeof(p.base.issuingDoor) - 1);
  memcpy(p.base.publicKey, keys.publicKey, 33);
  p.base.publicKeyLen = 33;
  strncpy(p.base.otherChainSource, "{DEST}", sizeof(p.base.otherChainSource) - 1);
  strncpy(p.base.attestationRewardAccount, keys.address, sizeof(p.base.attestationRewardAccount) - 1);
  strncpy(p.base.attestationSignerAccount, keys.address, sizeof(p.base.attestationSignerAccount) - 1);
  p.base.wasLockingChainSend = 1;
  p.base.amount.kind = xahau::AmountKind::XrpDrops;
  p.base.amount.drops = 10000000ULL;
  strncpy(p.destination, "{DEST}", sizeof(p.destination) - 1);
  p.signatureReward.kind = xahau::AmountKind::XrpDrops;
  p.signatureReward.drops = 100ULL;
"""
        + common_fee("p.base.common"),
        "XChainAddAccountCreateAttestation",
        True,
        "p.base.common",
    )

    lb = f"""  if (!xahau::sketch::parseHash256("{HASH32}", p.loanBrokerId)) {{
    Serial.println(F("Bad LoanBrokerID hex"));
    return;
  }}
"""
    add(E, "LoanBrokerSet", "LoanBrokerSet", 74, 2, "DevnetFirst — create/update loan broker.", lb + common_fee(), "LoanBrokerSet", True)
    add(E, "LoanBrokerDelete", "LoanBrokerDelete", 75, 2, "DevnetFirst — delete loan broker.", lb + common_fee(), "LoanBrokerDelete", True)
    add(
        E,
        "LoanBrokerCoverDeposit",
        "LoanBrokerCoverDeposit",
        76,
        2,
        "DevnetFirst — deposit first-loss cover.",
        lb
        + """  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 1000000ULL;
"""
        + common_fee(),
        "LoanBrokerCoverDeposit",
        True,
    )
    add(
        E,
        "LoanBrokerCoverWithdraw",
        "LoanBrokerCoverWithdraw",
        77,
        2,
        "DevnetFirst — withdraw cover.",
        lb
        + """  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 1000000ULL;
"""
        + common_fee(),
        "LoanBrokerCoverWithdraw",
        True,
    )
    add(
        E,
        "LoanBrokerCoverClawback",
        "LoanBrokerCoverClawback",
        78,
        2,
        "DevnetFirst — clawback cover.",
        lb
        + f"""  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 1000000ULL;
  strncpy(p.holder, "{DEST}", sizeof(p.holder) - 1);
"""
        + common_fee(),
        "LoanBrokerCoverClawback",
        True,
    )
    loan = f"""  p.hasBrokerId = true;
  if (!xahau::sketch::parseHash256("{HASH32}", p.loanBrokerId)) {{
    Serial.println(F("Bad LoanBrokerID hex"));
    return;
  }}
  p.hasLoanId = true;
  if (!xahau::sketch::parseHash256("{HASH32}", p.loanId)) {{
    Serial.println(F("Bad LoanID hex"));
    return;
  }}
"""
    add(
        E,
        "LoanSet",
        "LoanSet",
        80,
        2,
        "DevnetFirst — create/update loan.",
        loan + f"""  strncpy(p.counterparty, "{DEST}", sizeof(p.counterparty) - 1);\n""" + common_fee(),
        "LoanSet",
        True,
    )
    add(E, "LoanDelete", "LoanDelete", 81, 2, "DevnetFirst — delete loan.", loan + common_fee(), "LoanDelete", True)
    add(E, "LoanManage", "LoanManage", 82, 2, "DevnetFirst — manage loan.", loan + common_fee(), "LoanManage", True)
    add(
        E,
        "LoanPay",
        "LoanPay",
        84,
        2,
        "DevnetFirst — loan payment.",
        loan
        + """  p.amount.kind = xahau::AmountKind::XrpDrops;
  p.amount.drops = 1000000ULL;
"""
        + common_fee(),
        "LoanPay",
        True,
    )
    return E


def write_example(ex):
    group = TX_GROUP.get(ex["folder"], "Other")
    folder = TX / group / ex["folder"]
    folder.mkdir(parents=True, exist_ok=True)
    name = ex["builder"]
    struct = STRUCT[name]
    sign_block = (BUILD_SIGN_BLOCK if ex["use_build"] else FILL_SIGN_BLOCK).format(name=name)
    group_note = f" * Xahau group: {group} (see docs/references/protocol/transactions/types)\n *"
    header = HEADER.format(
        title=ex["title"], tx_type=ex["title"], code=ex["code"], live_note=ex["live"]
    )
    # Insert group line after Transaction type line
    header = header.replace(
        f" * Transaction type: {ex['title']} (catalog code {ex['code']})\n *",
        f" * Transaction type: {ex['title']} (catalog code {ex['code']})\n{group_note}",
    )
    body = (
        header
        + INCLUDES
        + CONFIG.format(net=ex["net"])
        + SETUP_HEAD.format(title=ex["title"])
        + f"  xahau::{struct} p;\n"
        + ex["params"]
        + SETUP_TAIL.format(sign_block=sign_block, common=ex["common"])
    )
    out = folder / f"{ex['folder']}.ino"
    out.write_text(body, encoding="utf-8")
    return out


def main():
    examples = build_examples()
    written = [write_example(ex) for ex in examples]
    # Prefer curated group README from reorganize_examples.py
    try:
        from reorganize_examples import write_readmes  # type: ignore

        write_readmes()
    except Exception:
        pass
    print(f"Wrote {len(written)} examples under {TX}/<Group>/<Name>/")
    print("Run reorganize_examples.py if you need to refresh group READMEs only.")


if __name__ == "__main__":
    main()
