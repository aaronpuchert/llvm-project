//===-- WindowsError.h - Support for mapping windows errors to posix-------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_WINDOWSERROR_H
#define LLVM_SUPPORT_WINDOWSERROR_H

#include "llvm/Support/Compiler.h"
#include <system_error>

namespace llvm {
LLVM_ABI std::error_code mapLastWindowsError();
LLVM_ABI std::error_code mapWindowsError(unsigned EV);
}

#endif
