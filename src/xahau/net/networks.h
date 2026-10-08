// networks.h - Xahau Mainnet / Testnet presets + live amendment matrix
// Protocol network IDs: Mainnet 21337, Testnet 21338 (NetworkID required on all txs).
// Amendment flags observed via https://xahau.network/rpc server_definitions
#pragma once

#include <stdint.h>
#include <string.h>

#include "../tx/tx_type.h"

namespace xahau {

/** Library preset index (not the on-wire NetworkID value). */
enum class NetworkId : uint8_t {
  Mainnet = 0,
  Testnet = 1,
  Custom = 255,
};

/** On-wire NetworkID values (must be signed into every Xahau transaction). */
constexpr uint32_t kProtocolNetworkIdMainnet = 21337u;
constexpr uint32_t kProtocolNetworkIdTestnet = 21338u;

struct NetworkEndpoint {
  NetworkId id;
  const char* name;  // "mainnet" | "testnet"

  // WebSocket (JSON-RPC over WS)
  const char* wsHost;
  uint16_t wsPort;
  const char* wsPath;
  bool wsUseSsl;

  // JSON-RPC HTTPS
  const char* rpcHost;
  uint16_t rpcPort;
  bool rpcUseSsl;

  const char* clioWsHost;
  uint16_t clioWsPort;

  const char* faucetHint;
  const char* notes;

  /** Value for TxCommon.networkId (signed field). */
  uint32_t protocolNetworkId;
};

/** Xahau Mainnet - production (XAH, real value). */
inline const NetworkEndpoint kNetworkMainnet = {
    NetworkId::Mainnet,
    "mainnet",
    /*ws*/ "xahau.network",
    443,
    "/",
    true,
    /*rpc*/ "xahau.network",
    443,
    true,
    /*clio*/ nullptr,
    0,
    /*faucet*/ nullptr,
    "Production Xahau. NetworkID 21337 required on every signed tx.",
    kProtocolNetworkIdMainnet};

/**
 * Xahau Testnet - app testing / faucets.
 * WSS: xahau-test.net:443
 */
inline const NetworkEndpoint kNetworkTestnet = {
    NetworkId::Testnet,
    "testnet",
    "xahau-test.net",
    443,
    "/",
    true,
    "xahau-test.net",
    443,
    true,
    nullptr,
    0,
    "https://xahau-test.net (faucet via ecosystem tools / Xaman)",
    "Testnet. NetworkID 21338 required. Fund accounts before submit.",
    kProtocolNetworkIdTestnet};

inline const NetworkEndpoint& networkPreset(NetworkId id) {
  switch (id) {
    case NetworkId::Mainnet:
      return kNetworkMainnet;
    case NetworkId::Testnet:
    default:
      return kNetworkTestnet;
  }
}

inline const NetworkEndpoint* networkPresetByName(const char* name) {
  if (!name) return nullptr;
  if (strcmp(name, "mainnet") == 0 || strcmp(name, "Mainnet") == 0) return &kNetworkMainnet;
  if (strcmp(name, "testnet") == 0 || strcmp(name, "Testnet") == 0) return &kNetworkTestnet;
  return nullptr;
}

inline uint32_t protocolNetworkIdFor(NetworkId id) {
  return networkPreset(id).protocolNetworkId;
}

// ---------------------------------------------------------------------------
// Amendment live status (server_definitions features, 2026-07-19 mainnet)
// ---------------------------------------------------------------------------

enum class AmendmentStatus : uint8_t {
  Enabled = 0,
  NotEnabled = 1,
  Absent = 2,
};

enum class AmendmentId : uint8_t {
  None = 0,
  Hooks,
  HooksUpdate1,
  HookCanEmit,
  ExtendedHookState,
  URIToken,
  Remit,
  Import,
  ZeroB2M,
  BalanceRewards,
  IOURewardClaim,
  Remarks,
  Touch,
  Cron,
  PriceOracle,
  Clawback,
  DeepFreeze,
  PaychanAndEscrowForTokens,
  XahauGenesis,
  ExpandedSignerList,
  DepositAuth,
  DepositPreauth,
  // Not enabled / encode-only as of snapshot
  NamedHooks,
  HooksUpdate2,
  HookOnV2,
  AMMClawback,
  AMM,
  DID,
  Credentials,
  MPTokensV1,
  PermissionedDomains,
  _Count
};

inline const char* amendmentName(AmendmentId a) {
  switch (a) {
    case AmendmentId::Hooks:
      return "Hooks";
    case AmendmentId::HooksUpdate1:
      return "HooksUpdate1";
    case AmendmentId::HookCanEmit:
      return "HookCanEmit";
    case AmendmentId::ExtendedHookState:
      return "ExtendedHookState";
    case AmendmentId::URIToken:
      return "URIToken";
    case AmendmentId::Remit:
      return "Remit";
    case AmendmentId::Import:
      return "Import";
    case AmendmentId::ZeroB2M:
      return "ZeroB2M";
    case AmendmentId::BalanceRewards:
      return "BalanceRewards";
    case AmendmentId::IOURewardClaim:
      return "IOURewardClaim";
    case AmendmentId::Remarks:
      return "Remarks";
    case AmendmentId::Touch:
      return "Touch";
    case AmendmentId::Cron:
      return "Cron";
    case AmendmentId::PriceOracle:
      return "PriceOracle";
    case AmendmentId::Clawback:
      return "Clawback";
    case AmendmentId::DeepFreeze:
      return "DeepFreeze";
    case AmendmentId::PaychanAndEscrowForTokens:
      return "PaychanAndEscrowForTokens";
    case AmendmentId::XahauGenesis:
      return "XahauGenesis";
    case AmendmentId::ExpandedSignerList:
      return "ExpandedSignerList";
    case AmendmentId::DepositAuth:
      return "DepositAuth";
    case AmendmentId::DepositPreauth:
      return "DepositPreauth";
    case AmendmentId::NamedHooks:
      return "NamedHooks";
    case AmendmentId::HooksUpdate2:
      return "HooksUpdate2";
    case AmendmentId::HookOnV2:
      return "HookOnV2";
    case AmendmentId::AMMClawback:
      return "AMMClawback";
    case AmendmentId::AMM:
      return "AMM";
    case AmendmentId::DID:
      return "DID";
    case AmendmentId::Credentials:
      return "Credentials";
    case AmendmentId::MPTokensV1:
      return "MPTokensV1";
    case AmendmentId::PermissionedDomains:
      return "PermissionedDomains";
    case AmendmentId::None:
    default:
      return "None";
  }
}

/**
 * Live amendment status. Mainnet and Testnet treated similarly for core Xahau
 * features; Prefer re-query feature RPC after upgrades.
 */
inline AmendmentStatus amendmentStatus(AmendmentId a, NetworkId net) {
  if (a == AmendmentId::None) return AmendmentStatus::Enabled;
  if (net == NetworkId::Custom) return AmendmentStatus::NotEnabled;

  using S = AmendmentStatus;
  switch (a) {
    // Confirmed enabled on mainnet snapshot
    case AmendmentId::Hooks:
    case AmendmentId::HooksUpdate1:
    case AmendmentId::HookCanEmit:
    case AmendmentId::ExtendedHookState:
    case AmendmentId::URIToken:
    case AmendmentId::Remit:
    case AmendmentId::Import:
    case AmendmentId::ZeroB2M:
    case AmendmentId::BalanceRewards:
    case AmendmentId::IOURewardClaim:
    case AmendmentId::Remarks:
    case AmendmentId::Touch:
    case AmendmentId::Cron:
    case AmendmentId::PriceOracle:
    case AmendmentId::Clawback:
    case AmendmentId::DeepFreeze:
    case AmendmentId::PaychanAndEscrowForTokens:
    case AmendmentId::XahauGenesis:
    case AmendmentId::ExpandedSignerList:
    case AmendmentId::DepositAuth:
    case AmendmentId::DepositPreauth:
      return S::Enabled;

    // In definitions / release but not enabled as of 2026-07-19 mainnet
    case AmendmentId::NamedHooks:
    case AmendmentId::HooksUpdate2:
    case AmendmentId::HookOnV2:
    case AmendmentId::AMMClawback:
      return S::NotEnabled;

    // Present in type table / XRPL port stubs; absent from Xahau testnet feature list (2026-09-26)
    case AmendmentId::AMM:
      return S::NotEnabled;
    case AmendmentId::DID:
    case AmendmentId::Credentials:
    case AmendmentId::MPTokensV1:
    case AmendmentId::PermissionedDomains:
      return S::Absent;

    default:
      return S::Enabled;
  }
}

inline bool amendmentEnabledOn(AmendmentId a, NetworkId net) {
  return amendmentStatus(a, net) == AmendmentStatus::Enabled;
}

inline AmendmentId txPrimaryAmendment(TxType t) {
  switch (t) {
    case TxType::SetHook:
    case TxType::Invoke:
      return AmendmentId::Hooks;
    case TxType::CronSet:
    case TxType::Cron:
      return AmendmentId::Cron;
    case TxType::URITokenMint:
    case TxType::URITokenBurn:
    case TxType::URITokenBuy:
    case TxType::URITokenCreateSellOffer:
    case TxType::URITokenCancelSellOffer:
      return AmendmentId::URIToken;
    case TxType::Remit:
      return AmendmentId::Remit;
    case TxType::Import:
      return AmendmentId::Import;
    case TxType::ClaimReward:
      return AmendmentId::BalanceRewards;
    case TxType::SetRemarks:
      return AmendmentId::Remarks;
    case TxType::OracleSet:
    case TxType::OracleDelete:
      return AmendmentId::PriceOracle;
    case TxType::Clawback:
      return AmendmentId::Clawback;
    case TxType::AMMCreate:
    case TxType::AMMDeposit:
    case TxType::AMMWithdraw:
    case TxType::AMMVote:
    case TxType::AMMBid:
    case TxType::AMMDelete:
      return AmendmentId::AMM;
    case TxType::AMMClawback:
      return AmendmentId::AMMClawback;
    case TxType::DIDSet:
    case TxType::DIDDelete:
      return AmendmentId::DID;
    case TxType::CredentialCreate:
    case TxType::CredentialAccept:
    case TxType::CredentialDelete:
      return AmendmentId::Credentials;
    case TxType::MPTokenIssuanceCreate:
    case TxType::MPTokenIssuanceDestroy:
    case TxType::MPTokenIssuanceSet:
    case TxType::MPTokenAuthorize:
      return AmendmentId::MPTokensV1;
    case TxType::PermissionedDomainSet:
    case TxType::PermissionedDomainDelete:
      return AmendmentId::PermissionedDomains;
    default:
      return AmendmentId::None;
  }
}

enum class TxLiveSupport : uint8_t {
  AllPublic = 0,
  EncodeOnly = 1,
  NotUserSubmit = 2,
};

inline TxLiveSupport txLiveSupport(TxType t) {
  switch (t) {
    case TxType::EnableAmendment:
    case TxType::SetFee:
    case TxType::UNLModify:
    case TxType::EmitFailure:
    case TxType::UNLReport:
    case TxType::GenesisMint:
    case TxType::Cron:
      return TxLiveSupport::NotUserSubmit;
    case TxType::AMMCreate:
    case TxType::AMMDeposit:
    case TxType::AMMWithdraw:
    case TxType::AMMVote:
    case TxType::AMMBid:
    case TxType::AMMDelete:
    case TxType::AMMClawback:
    case TxType::DIDSet:
    case TxType::DIDDelete:
    case TxType::CredentialCreate:
    case TxType::CredentialAccept:
    case TxType::CredentialDelete:
    case TxType::MPTokenIssuanceCreate:
    case TxType::MPTokenIssuanceDestroy:
    case TxType::MPTokenIssuanceSet:
    case TxType::MPTokenAuthorize:
    case TxType::PermissionedDomainSet:
    case TxType::PermissionedDomainDelete:
    case TxType::Import:
    case TxType::NFTokenMint:
    case TxType::NFTokenBurn:
    case TxType::NFTokenCreateOffer:
    case TxType::NFTokenCancelOffer:
    case TxType::NFTokenAcceptOffer:
    case TxType::NFTokenModify:
    // XRPL-port stubs (library type codes 200+) - not live on Xahau public nets
    case TxType::Batch:
    case TxType::DelegateSet:
    case TxType::VaultCreate:
    case TxType::VaultSet:
    case TxType::VaultDelete:
    case TxType::VaultDeposit:
    case TxType::VaultWithdraw:
    case TxType::VaultClawback:
    case TxType::LoanBrokerSet:
    case TxType::LoanBrokerDelete:
    case TxType::LoanBrokerCoverDeposit:
    case TxType::LoanBrokerCoverWithdraw:
    case TxType::LoanBrokerCoverClawback:
    case TxType::LoanSet:
    case TxType::LoanDelete:
    case TxType::LoanManage:
    case TxType::LoanPay:
    // XChain* intentional gap / encode stubs
    case TxType::XChainCreateBridge:
    case TxType::XChainCreateClaimID:
    case TxType::XChainCommit:
    case TxType::XChainClaim:
    case TxType::XChainAccountCreateCommit:
    case TxType::XChainAddClaimAttestation:
    case TxType::XChainAddAccountCreateAttestation:
    case TxType::XChainModifyBridge:
      return TxLiveSupport::EncodeOnly;
    default:
      return TxLiveSupport::AllPublic;
  }
}

inline bool isTxLiveEnabledOn(TxType t, NetworkId net) {
  if (txLiveSupport(t) != TxLiveSupport::AllPublic) return false;
  return amendmentEnabledOn(txPrimaryAmendment(t), net);
}

inline bool isTxLiveEnabledOn(const char* txTypeName, NetworkId net) {
  const TxTypeInfo* info = txTypeLookupByName(txTypeName);
  if (!info) return true;
  return isTxLiveEnabledOn(info->type, net);
}

inline NetworkId recommendedLiveNetwork(TxType t) {
  if (txLiveSupport(t) == TxLiveSupport::NotUserSubmit ||
      txLiveSupport(t) == TxLiveSupport::EncodeOnly) {
    return NetworkId::Custom;
  }
  // Prefer testnet for smoke; Oracle + Hooks live on mainnet too.
  return NetworkId::Testnet;
}
inline bool networkEndpointValid(const NetworkEndpoint& ep) {
  if (!ep.name || !ep.name[0]) return false;
  if (!ep.wsHost || !ep.wsHost[0]) return false;
  if (ep.wsPort == 0) return false;
  if (!ep.wsPath) return false;
  if (!ep.rpcHost || !ep.rpcHost[0]) return false;
  if (ep.rpcPort == 0) return false;
  if (ep.protocolNetworkId < 1025u) return false;
  return true;
}
inline bool allPublicNetworksWired() {
  return networkEndpointValid(kNetworkMainnet) && networkEndpointValid(kNetworkTestnet);
}

}  // namespace xahau
