// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "processenv.h"
#include "runtime/importobj.h"

namespace WasmEdge {
namespace Host {

class WasmEdgeProcessModule : public Runtime::ImportObject {
public:
  WasmEdgeProcessModule();

  WasmEdgeProcessEnvironment &getEnv() { return Env; }

private:
  WasmEdgeProcessEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
