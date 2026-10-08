// tx_type.cpp — full Xahau transaction catalog (live type codes)
#include "tx_type.h"

#include <string.h>

namespace xahau {

namespace {

const TxTypeInfo kCatalog[] = {
    {TxType::Payment, 0, "Payment", true, TxMaturity::Golden},
    {TxType::EscrowCreate, 1, "EscrowCreate", true, TxMaturity::Golden},
    {TxType::EscrowFinish, 2, "EscrowFinish", true, TxMaturity::Golden},
    {TxType::AccountSet, 3, "AccountSet", true, TxMaturity::Golden},
    {TxType::NicknameSet, 6, "NicknameSet", false, TxMaturity::Catalog},
    {TxType::EscrowCancel, 4, "EscrowCancel", true, TxMaturity::Golden},
    {TxType::SetRegularKey, 5, "SetRegularKey", true, TxMaturity::Golden},
    {TxType::OfferCreate, 7, "OfferCreate", true, TxMaturity::Golden},
    {TxType::Contract, 9, "Contract", false, TxMaturity::Catalog},
    {TxType::SpinalTap, 11, "SpinalTap", false, TxMaturity::Catalog},
    {TxType::OfferCancel, 8, "OfferCancel", true, TxMaturity::Golden},
    {TxType::TicketCreate, 10, "TicketCreate", true, TxMaturity::Golden},
    {TxType::SignerListSet, 12, "SignerListSet", true, TxMaturity::Golden},
    {TxType::PaymentChannelCreate, 13, "PaymentChannelCreate", true, TxMaturity::Golden},
    {TxType::PaymentChannelFund, 14, "PaymentChannelFund", true, TxMaturity::Golden},
    {TxType::PaymentChannelClaim, 15, "PaymentChannelClaim", true, TxMaturity::Golden},
    {TxType::CheckCreate, 16, "CheckCreate", true, TxMaturity::Golden},
    {TxType::CheckCash, 17, "CheckCash", true, TxMaturity::Golden},
    {TxType::CheckCancel, 18, "CheckCancel", true, TxMaturity::Golden},
    {TxType::DepositPreauth, 19, "DepositPreauth", true, TxMaturity::Golden},
    {TxType::TrustSet, 20, "TrustSet", true, TxMaturity::Golden},
    {TxType::AccountDelete, 21, "AccountDelete", true, TxMaturity::Golden},

    {TxType::SetHook, 22, "SetHook", true, TxMaturity::Golden},

    {TxType::NFTokenMint, 25, "NFTokenMint", true, TxMaturity::Catalog},
    {TxType::NFTokenBurn, 26, "NFTokenBurn", true, TxMaturity::Catalog},
    {TxType::NFTokenCreateOffer, 27, "NFTokenCreateOffer", true, TxMaturity::Catalog},
    {TxType::NFTokenCancelOffer, 28, "NFTokenCancelOffer", true, TxMaturity::Catalog},
    {TxType::NFTokenAcceptOffer, 29, "NFTokenAcceptOffer", true, TxMaturity::Catalog},
    {TxType::Clawback, 30, "Clawback", true, TxMaturity::Golden},
    {TxType::AMMClawback, 31, "AMMClawback", true, TxMaturity::Catalog},
    {TxType::AMMCreate, 35, "AMMCreate", true, TxMaturity::Catalog},
    {TxType::AMMDeposit, 36, "AMMDeposit", true, TxMaturity::Catalog},
    {TxType::AMMWithdraw, 37, "AMMWithdraw", true, TxMaturity::Catalog},
    {TxType::AMMVote, 38, "AMMVote", true, TxMaturity::Catalog},
    {TxType::AMMBid, 39, "AMMBid", true, TxMaturity::Catalog},
    {TxType::AMMDelete, 40, "AMMDelete", true, TxMaturity::Catalog},

    {TxType::URITokenMint, 45, "URITokenMint", true, TxMaturity::Golden},
    {TxType::URITokenBurn, 46, "URITokenBurn", true, TxMaturity::Golden},
    {TxType::URITokenBuy, 47, "URITokenBuy", true, TxMaturity::Golden},
    {TxType::URITokenCreateSellOffer, 48, "URITokenCreateSellOffer", true, TxMaturity::Golden},
    {TxType::URITokenCancelSellOffer, 49, "URITokenCancelSellOffer", true, TxMaturity::Golden},

    {TxType::XChainCreateClaimID, 50, "XChainCreateClaimID", true, TxMaturity::Catalog},
    {TxType::XChainCommit, 51, "XChainCommit", true, TxMaturity::Catalog},
    {TxType::XChainClaim, 52, "XChainClaim", true, TxMaturity::Catalog},
    {TxType::XChainAccountCreateCommit, 53, "XChainAccountCreateCommit", true, TxMaturity::Catalog},
    {TxType::XChainAddClaimAttestation, 54, "XChainAddClaimAttestation", true, TxMaturity::Catalog},
    {TxType::XChainAddAccountCreateAttestation, 55, "XChainAddAccountCreateAttestation", true,
     TxMaturity::Catalog},
    {TxType::XChainModifyBridge, 56, "XChainModifyBridge", true, TxMaturity::Catalog},
    {TxType::XChainCreateBridge, 57, "XChainCreateBridge", true, TxMaturity::Catalog},

    {TxType::DIDSet, 58, "DIDSet", true, TxMaturity::Catalog},
    {TxType::DIDDelete, 59, "DIDDelete", true, TxMaturity::Catalog},
    {TxType::OracleSet, 60, "OracleSet", true, TxMaturity::Golden},
    {TxType::OracleDelete, 61, "OracleDelete", true, TxMaturity::Golden},

    {TxType::LedgerStateFix, 62, "LedgerStateFix", false, TxMaturity::Catalog},
    {TxType::MPTokenIssuanceCreate, 63, "MPTokenIssuanceCreate", true, TxMaturity::Catalog},
    {TxType::MPTokenIssuanceDestroy, 64, "MPTokenIssuanceDestroy", true, TxMaturity::Catalog},
    {TxType::MPTokenIssuanceSet, 65, "MPTokenIssuanceSet", true, TxMaturity::Catalog},
    {TxType::MPTokenAuthorize, 66, "MPTokenAuthorize", true, TxMaturity::Catalog},
    {TxType::CredentialCreate, 67, "CredentialCreate", true, TxMaturity::Catalog},
    {TxType::CredentialAccept, 68, "CredentialAccept", true, TxMaturity::Catalog},
    {TxType::CredentialDelete, 69, "CredentialDelete", true, TxMaturity::Catalog},
    {TxType::NFTokenModify, 70, "NFTokenModify", true, TxMaturity::Catalog},
    {TxType::PermissionedDomainSet, 71, "PermissionedDomainSet", true, TxMaturity::Catalog},
    {TxType::PermissionedDomainDelete, 72, "PermissionedDomainDelete", true, TxMaturity::Catalog},

    // Ported XRPL-only builders (encode offline; not Xahau mainnet live types)
    {TxType::Batch, 200, "Batch", false, TxMaturity::Catalog},
    {TxType::DelegateSet, 201, "DelegateSet", false, TxMaturity::Catalog},
    {TxType::VaultCreate, 202, "VaultCreate", false, TxMaturity::Catalog},
    {TxType::VaultSet, 203, "VaultSet", false, TxMaturity::Catalog},
    {TxType::VaultDelete, 204, "VaultDelete", false, TxMaturity::Catalog},
    {TxType::VaultDeposit, 205, "VaultDeposit", false, TxMaturity::Catalog},
    {TxType::VaultWithdraw, 206, "VaultWithdraw", false, TxMaturity::Catalog},
    {TxType::VaultClawback, 207, "VaultClawback", false, TxMaturity::Catalog},
    {TxType::LoanBrokerSet, 208, "LoanBrokerSet", false, TxMaturity::Catalog},
    {TxType::LoanBrokerDelete, 209, "LoanBrokerDelete", false, TxMaturity::Catalog},
    {TxType::LoanBrokerCoverDeposit, 210, "LoanBrokerCoverDeposit", false, TxMaturity::Catalog},
    {TxType::LoanBrokerCoverWithdraw, 211, "LoanBrokerCoverWithdraw", false, TxMaturity::Catalog},
    {TxType::LoanBrokerCoverClawback, 212, "LoanBrokerCoverClawback", false, TxMaturity::Catalog},
    {TxType::LoanSet, 213, "LoanSet", false, TxMaturity::Catalog},
    {TxType::LoanDelete, 214, "LoanDelete", false, TxMaturity::Catalog},
    {TxType::LoanManage, 215, "LoanManage", false, TxMaturity::Catalog},
    {TxType::LoanPay, 216, "LoanPay", false, TxMaturity::Catalog},

    {TxType::Cron, 92, "Cron", false, TxMaturity::Catalog},
    {TxType::CronSet, 93, "CronSet", true, TxMaturity::Golden},
    {TxType::SetRemarks, 94, "SetRemarks", true, TxMaturity::Golden},
    {TxType::Remit, 95, "Remit", true, TxMaturity::Golden},
    {TxType::GenesisMint, 96, "GenesisMint", false, TxMaturity::Catalog},
    {TxType::Import, 97, "Import", true, TxMaturity::Encode},
    {TxType::ClaimReward, 98, "ClaimReward", true, TxMaturity::Golden},
    {TxType::Invoke, 99, "Invoke", true, TxMaturity::Golden},

    {TxType::EnableAmendment, 100, "EnableAmendment", false, TxMaturity::Catalog},
    {TxType::SetFee, 101, "SetFee", false, TxMaturity::Catalog},
    {TxType::UNLModify, 102, "UNLModify", false, TxMaturity::Catalog},
    {TxType::EmitFailure, 103, "EmitFailure", false, TxMaturity::Catalog},
    {TxType::UNLReport, 104, "UNLReport", false, TxMaturity::Catalog},
};

constexpr size_t kCount = sizeof(kCatalog) / sizeof(kCatalog[0]);

}  // namespace

const TxTypeInfo* txTypeCatalog() { return kCatalog; }

size_t txTypeCatalogCount() { return kCount; }

const TxTypeInfo* txTypeLookup(TxType t) {
  for (size_t i = 0; i < kCount; ++i) {
    if (kCatalog[i].type == t) return &kCatalog[i];
  }
  return nullptr;
}

const TxTypeInfo* txTypeLookupByCode(uint16_t typeCode) {
  for (size_t i = 0; i < kCount; ++i) {
    if (kCatalog[i].typeCode == typeCode) return &kCatalog[i];
  }
  return nullptr;
}

const TxTypeInfo* txTypeLookupByName(const char* name) {
  if (!name) return nullptr;
  for (size_t i = 0; i < kCount; ++i) {
    if (strcmp(kCatalog[i].name, name) == 0) return &kCatalog[i];
  }
  return nullptr;
}

const char* txTypeName(TxType t) {
  const TxTypeInfo* info = txTypeLookup(t);
  return info ? info->name : "Invalid";
}

}  // namespace xahau
