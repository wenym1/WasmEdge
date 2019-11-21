#include "vm/hostfunc/onnc/runtime_gemm_float.h"
#include "executor/common.h"
#include "executor/worker/util.h"
#include "onnc/onnc_runtime.h"

#include <stdint.h>

namespace SSVM {
namespace Executor {

ONNCRuntimeGemmFloat::ONNCRuntimeGemmFloat() {
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::F32);
  appendParamDef(AST::ValType::F32);
  appendParamDef(AST::ValType::I32);
  appendParamDef(AST::ValType::I32);
}

ErrCode ONNCRuntimeGemmFloat::run(std::vector<Value> &Args,
                                  std::vector<Value> &Res, StoreManager &Store,
                                  Instance::ModuleInstance *ModInst) {
  /// Arg: void* onnc_runtime_context,
  ///      const float *input_A,
  ///      int32_t input_A_ndim,
  ///      const int32_t *input_A_dims,
  ///      const float *input_B,
  ///      int32_t input_B_ndim,
  ///      const int32_t *input_B_dims,
  ///      const float *input_C,
  ///      int32_t input_C_ndim,
  ///      const int32_t *input_C_dims,
  ///      float *output_Y,
  ///      int32_t output_Y_ndim,
  ///      const int32_t *output_Y_dims,
  ///      float alpha,
  ///      float beta,
  ///      int32_t transA,
  ///      int32_t transB
  if (Args.size() != 17) {
    return ErrCode::CallFunctionError;
  }
  ErrCode Status = ErrCode::Success;
  unsigned int RuntimeContextPtr = retrieveValue<uint32_t>(Args[16]);
  unsigned int InAPtr = retrieveValue<uint32_t>(Args[15]);
  unsigned int InANDim = retrieveValue<uint32_t>(Args[14]);
  unsigned int InADimsPtr = retrieveValue<uint32_t>(Args[13]);
  unsigned int InBPtr = retrieveValue<uint32_t>(Args[12]);
  unsigned int InBNDim = retrieveValue<uint32_t>(Args[11]);
  unsigned int InBDimsPtr = retrieveValue<uint32_t>(Args[10]);
  unsigned int InCPtr = retrieveValue<uint32_t>(Args[9]);
  unsigned int InCNDim = retrieveValue<uint32_t>(Args[8]);
  unsigned int InCDimsPtr = retrieveValue<uint32_t>(Args[7]);
  unsigned int OutYPtr = retrieveValue<uint32_t>(Args[6]);
  unsigned int OutYNDim = retrieveValue<uint32_t>(Args[5]);
  unsigned int OutYDimsPtr = retrieveValue<uint32_t>(Args[4]);
  float Alpha = retrieveValue<float>(Args[3]);
  float Beta = retrieveValue<float>(Args[2]);
  unsigned int TransA = retrieveValue<uint32_t>(Args[1]);
  unsigned int TransB = retrieveValue<uint32_t>(Args[0]);

  /// Get memory instance.
  unsigned int MemoryAddr = 0;
  Instance::MemoryInstance *MemInst = nullptr;
  if ((Status = ModInst->getMemAddr(0, MemoryAddr)) != ErrCode::Success) {
    return Status;
  }
  if ((Status = Store.getMemory(MemoryAddr, MemInst)) != ErrCode::Success) {
    return Status;
  }

  void *RuntimeContext =
      reinterpret_cast<void *>(MemInst->getPointer(RuntimeContextPtr));
  int32_t *InADims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(InADimsPtr));
  int32_t *InBDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(InBDimsPtr));
  int32_t *InCDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(InCDimsPtr));
  int32_t *OutYDims =
      reinterpret_cast<int32_t *>(MemInst->getPointer(OutYDimsPtr));
  float *InA = reinterpret_cast<float *>(MemInst->getPointer(InAPtr));
  float *InB = reinterpret_cast<float *>(MemInst->getPointer(InBPtr));
  float *InC = reinterpret_cast<float *>(MemInst->getPointer(InCPtr));
  float *OutY = reinterpret_cast<float *>(MemInst->getPointer(OutYPtr));

  ONNC_RUNTIME_gemm_float(RuntimeContext, InA, InANDim, InADims, InB, InBNDim,
                          InBDims, InC, InCNDim, InCDims, OutY, OutYNDim,
                          OutYDims, Alpha, Beta, TransA, TransB);

  /// Return: void
  return Status;
}

} // namespace Executor
} // namespace SSVM