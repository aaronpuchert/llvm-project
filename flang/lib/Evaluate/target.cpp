//===-- lib/Semantics/target.cpp ------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "flang/Evaluate/target.h"
#include "flang/Common/template.h"
#include "flang/Common/type-kinds.h"
#include "flang/Evaluate/common.h"
#include "flang/Evaluate/type.h"

namespace Fortran::evaluate {

Rounding TargetCharacteristics::defaultRounding;

TargetCharacteristics::TargetCharacteristics() {
  auto enableCategoryKinds{[this](TypeCategory category) {
    for (int kind{1}; kind <= maxKind; ++kind) {
      if (CanSupportType(category, kind)) {
        auto byteSize{
            static_cast<std::size_t>(common::TypeSizeInBytes(category, kind))};
        std::size_t align{byteSize};
        if (category == TypeCategory::Complex) {
          align /= 2;
        }
        EnableType(category, kind, byteSize, align);
      }
    }
  }};
  enableCategoryKinds(TypeCategory::Integer);
  enableCategoryKinds(TypeCategory::Real);
  enableCategoryKinds(TypeCategory::Complex);
  enableCategoryKinds(TypeCategory::Character);
  enableCategoryKinds(TypeCategory::Logical);
  enableCategoryKinds(TypeCategory::Unsigned);

  isBigEndian_ = !isHostLittleEndian;

  areSubnormalsFlushedToZero_ = false;
}

bool TargetCharacteristics::CanSupportType(
    TypeCategory category, std::int64_t kind) {
  return common::IsValidKindOfIntrinsicType(category, kind);
}

bool TargetCharacteristics::EnableType(common::TypeCategory category,
    std::int64_t kind, std::size_t byteSize, std::size_t align) {
  if (CanSupportType(category, kind)) {
    byteSize_[static_cast<int>(category)][kind] = byteSize;
    align_[static_cast<int>(category)][kind] = align;
    maxByteSize_ = std::max(maxByteSize_, byteSize);
    maxAlignment_ = std::max(maxAlignment_, align);
    return true;
  } else {
    return false;
  }
}

void TargetCharacteristics::DisableType(
    common::TypeCategory category, std::int64_t kind) {
  if (kind > 0 && kind <= maxKind) {
    align_[static_cast<int>(category)][kind] = 0;
  }
}

std::size_t TargetCharacteristics::GetByteSize(
    common::TypeCategory category, std::int64_t kind) const {
  if (kind > 0 && kind <= maxKind) {
    return byteSize_[static_cast<int>(category)][kind];
  } else {
    return 0;
  }
}

std::size_t TargetCharacteristics::GetAlignment(
    common::TypeCategory category, std::int64_t kind) const {
  if (kind > 0 && kind <= maxKind) {
    return align_[static_cast<int>(category)][kind];
  } else {
    return 0;
  }
}

bool TargetCharacteristics::IsTypeEnabled(
    common::TypeCategory category, std::int64_t kind) const {
  return GetAlignment(category, kind) > 0;
}

void TargetCharacteristics::set_isBigEndian(bool isBig) {
  isBigEndian_ = isBig;
}

void TargetCharacteristics::set_isPPC(bool isPowerPC) { isPPC_ = isPowerPC; }
void TargetCharacteristics::set_isSPARC(bool isSPARC) { isSPARC_ = isSPARC; }

void TargetCharacteristics::set_areSubnormalsFlushedToZero(bool yes) {
  areSubnormalsFlushedToZero_ = yes;
}

// Check if a given real kind has flushing control.
bool TargetCharacteristics::hasSubnormalFlushingControl(int kind) const {
  CHECK(kind > 0 && kind <= maxKind);
  CHECK(CanSupportType(TypeCategory::Real, kind));
  return hasSubnormalFlushingControl_[kind];
}

// Check if any or all real kinds have flushing control.
bool TargetCharacteristics::hasSubnormalFlushingControl(bool any) const {
  for (int kind{1}; kind <= maxKind; ++kind) {
    if (CanSupportType(TypeCategory::Real, kind) &&
        hasSubnormalFlushingControl_[kind] == any) {
      return any;
    }
  }
  return !any;
}

void TargetCharacteristics::set_hasSubnormalFlushingControl(
    int kind, bool yes) {
  CHECK(kind > 0 && kind <= maxKind);
  hasSubnormalFlushingControl_[kind] = yes;
}

// Check if a given real kind has (nonstandard) ieee_denorm exception control.
bool TargetCharacteristics::hasSubnormalExceptionSupport(int kind) const {
  CHECK(kind > 0 && kind <= maxKind);
  CHECK(CanSupportType(TypeCategory::Real, kind));
  return hasSubnormalExceptionSupport_[kind];
}

// Check if all real kinds have support for the ieee_denorm exception.
bool TargetCharacteristics::hasSubnormalExceptionSupport() const {
  for (int kind{1}; kind <= maxKind; ++kind) {
    if (CanSupportType(TypeCategory::Real, kind) &&
        !hasSubnormalExceptionSupport_[kind]) {
      return false;
    }
  }
  return true;
}

void TargetCharacteristics::set_hasSubnormalExceptionSupport(
    int kind, bool yes) {
  CHECK(kind > 0 && kind <= maxKind);
  hasSubnormalExceptionSupport_[kind] = yes;
}

void TargetCharacteristics::set_roundingMode(Rounding rounding) {
  roundingMode_ = rounding;
}

// SELECTED_INT_KIND() -- F'2018 16.9.169
// and SELECTED_UNSIGNED_KIND() extension (same results)
class SelectedIntKindVisitor {
public:
  SelectedIntKindVisitor(
      const TargetCharacteristics &targetCharacteristics, std::int64_t p)
      : targetCharacteristics_{targetCharacteristics}, precision_{p} {}
  using Result = std::optional<int>;
  using Types = IntegerTypes;
  template <typename T> Result Test() const {
    if (Scalar<T>::RANGE >= precision_ &&
        targetCharacteristics_.IsTypeEnabled(T::category, T::kind)) {
      return T::kind;
    } else {
      return std::nullopt;
    }
  }

private:
  const TargetCharacteristics &targetCharacteristics_;
  std::int64_t precision_;
};

int TargetCharacteristics::SelectedIntKind(std::int64_t precision) const {
  if (auto kind{
          common::SearchTypes(SelectedIntKindVisitor{*this, precision})}) {
    return *kind;
  } else {
    return -1;
  }
}

// SELECTED_LOGICAL_KIND() -- F'2023 16.9.182
class SelectedLogicalKindVisitor {
public:
  SelectedLogicalKindVisitor(
      const TargetCharacteristics &targetCharacteristics, std::int64_t bits)
      : targetCharacteristics_{targetCharacteristics}, bits_{bits} {}
  using Result = std::optional<int>;
  using Types = LogicalTypes;
  template <typename T> Result Test() const {
    if (Scalar<T>::bits >= bits_ &&
        targetCharacteristics_.IsTypeEnabled(T::category, T::kind)) {
      return T::kind;
    } else {
      return std::nullopt;
    }
  }

private:
  const TargetCharacteristics &targetCharacteristics_;
  std::int64_t bits_;
};

int TargetCharacteristics::SelectedLogicalKind(std::int64_t bits) const {
  if (auto kind{common::SearchTypes(SelectedLogicalKindVisitor{*this, bits})}) {
    return *kind;
  } else {
    return -1;
  }
}

// SELECTED_REAL_KIND() -- F'2018 16.9.170
class SelectedRealKindVisitor {
public:
  SelectedRealKindVisitor(const TargetCharacteristics &targetCharacteristics,
      std::int64_t p, std::int64_t r)
      : targetCharacteristics_{targetCharacteristics}, precision_{p}, range_{
                                                                          r} {}
  using Result = std::optional<int>;
  using Types = RealTypes;
  template <typename T> Result Test() const {
    if (Scalar<T>::PRECISION >= precision_ && Scalar<T>::RANGE >= range_ &&
        targetCharacteristics_.IsTypeEnabled(T::category, T::kind)) {
      return {T::kind};
    } else {
      return std::nullopt;
    }
  }

private:
  const TargetCharacteristics &targetCharacteristics_;
  std::int64_t precision_, range_;
};

int TargetCharacteristics::SelectedRealKind(
    std::int64_t precision, std::int64_t range, std::int64_t radix) const {
  if (radix != 2) {
    return -5;
  }
  if (auto kind{common::SearchTypes(
          SelectedRealKindVisitor{*this, precision, range})}) {
    return *kind;
  }
  // No kind has both sufficient precision and sufficient range.
  // The negative return value encodes whether any kinds exist that
  // could satisfy either constraint independently.
  bool pOK{common::SearchTypes(SelectedRealKindVisitor{*this, precision, 0})};
  bool rOK{common::SearchTypes(SelectedRealKindVisitor{*this, 0, range})};
  if (pOK) {
    if (rOK) {
      return -4;
    } else {
      return -2;
    }
  } else {
    if (rOK) {
      return -1;
    } else {
      return -3;
    }
  }
}

} // namespace Fortran::evaluate
