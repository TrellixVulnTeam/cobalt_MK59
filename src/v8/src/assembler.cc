// Copyright (c) 1994-2006 Sun Microsystems Inc.
// All Rights Reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// - Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// - Redistribution in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// - Neither the name of Sun Microsystems or the names of contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// The original source code covered by the above license above has been
// modified significantly by Google Inc.
// Copyright 2012 the V8 project authors. All rights reserved.

#include "src/assembler.h"

#include <math.h>
#include <string.h>
#include <cmath>

#include "src/api.h"
#include "src/assembler-inl.h"
#include "src/base/cpu.h"
#include "src/base/functional.h"
#include "src/base/ieee754.h"
#include "src/base/lazy-instance.h"
#include "src/base/platform/platform.h"
#include "src/base/utils/random-number-generator.h"
#include "src/codegen.h"
#include "src/compiler/code-assembler.h"
#include "src/counters.h"
#include "src/debug/debug.h"
#include "src/deoptimizer.h"
#include "src/disassembler.h"
#include "src/elements.h"
#include "src/execution.h"
#include "src/ic/ic.h"
#include "src/ic/stub-cache.h"
#include "src/interpreter/bytecodes.h"
#include "src/interpreter/interpreter.h"
#include "src/isolate.h"
#include "src/ostreams.h"
#include "src/regexp/jsregexp.h"
#include "src/regexp/regexp-macro-assembler.h"
#include "src/regexp/regexp-stack.h"
#include "src/register-configuration.h"
#include "src/runtime/runtime.h"
#include "src/simulator.h"  // For flushing instruction cache.
#include "src/snapshot/serializer-common.h"
#include "src/string-search.h"
#include "src/wasm/wasm-external-refs.h"

// Include native regexp-macro-assembler.
#ifndef V8_INTERPRETED_REGEXP
#if V8_TARGET_ARCH_IA32
#include "src/regexp/ia32/regexp-macro-assembler-ia32.h"  // NOLINT
#elif V8_TARGET_ARCH_X64
#include "src/regexp/x64/regexp-macro-assembler-x64.h"  // NOLINT
#elif V8_TARGET_ARCH_ARM64
#include "src/regexp/arm64/regexp-macro-assembler-arm64.h"  // NOLINT
#elif V8_TARGET_ARCH_ARM
#include "src/regexp/arm/regexp-macro-assembler-arm.h"  // NOLINT
#elif V8_TARGET_ARCH_PPC
#include "src/regexp/ppc/regexp-macro-assembler-ppc.h"  // NOLINT
#elif V8_TARGET_ARCH_MIPS
#include "src/regexp/mips/regexp-macro-assembler-mips.h"  // NOLINT
#elif V8_TARGET_ARCH_MIPS64
#include "src/regexp/mips64/regexp-macro-assembler-mips64.h"  // NOLINT
#elif V8_TARGET_ARCH_S390
#include "src/regexp/s390/regexp-macro-assembler-s390.h"  // NOLINT
#else  // Unknown architecture.
#error "Unknown architecture."
#endif  // Target architecture.
#endif  // V8_INTERPRETED_REGEXP

#ifdef V8_INTL_SUPPORT
#include "src/intl.h"
#endif  // V8_INTL_SUPPORT

namespace v8 {
namespace internal {

// -----------------------------------------------------------------------------
// Common double constants.

struct DoubleConstant BASE_EMBEDDED {
double min_int;
double one_half;
double minus_one_half;
double negative_infinity;
uint64_t the_hole_nan;
double uint32_bias;
};

static DoubleConstant double_constants;

static struct V8_ALIGNED(16) {
  uint32_t a;
  uint32_t b;
  uint32_t c;
  uint32_t d;
} float_absolute_constant = {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};

static struct V8_ALIGNED(16) {
  uint32_t a;
  uint32_t b;
  uint32_t c;
  uint32_t d;
} float_negate_constant = {0x80000000, 0x80000000, 0x80000000, 0x80000000};

static struct V8_ALIGNED(16) {
  uint64_t a;
  uint64_t b;
} double_absolute_constant = {uint64_t{0x7FFFFFFFFFFFFFFF},
                              uint64_t{0x7FFFFFFFFFFFFFFF}};

static struct V8_ALIGNED(16) {
  uint64_t a;
  uint64_t b;
} double_negate_constant = {uint64_t{0x8000000000000000},
                            uint64_t{0x8000000000000000}};

const char* const RelocInfo::kFillerCommentString = "DEOPTIMIZATION PADDING";

// -----------------------------------------------------------------------------
// Implementation of AssemblerBase

AssemblerBase::IsolateData::IsolateData(Isolate* isolate)
    : serializer_enabled_(isolate->serializer_enabled())
#if V8_TARGET_ARCH_X64
      ,
      code_range_start_(
          isolate->heap()->memory_allocator()->code_range()->start())
#endif
{
}

AssemblerBase::AssemblerBase(IsolateData isolate_data, void* buffer,
                             int buffer_size)
    : isolate_data_(isolate_data),
      enabled_cpu_features_(0),
      emit_debug_code_(FLAG_debug_code),
      predictable_code_size_(false),
      constant_pool_available_(false),
      jump_optimization_info_(nullptr) {
  own_buffer_ = buffer == nullptr;
  if (buffer_size == 0) buffer_size = kMinimalBufferSize;
  DCHECK_GT(buffer_size, 0);
  if (own_buffer_) buffer = NewArray<byte>(buffer_size);
  buffer_ = static_cast<byte*>(buffer);
  buffer_size_ = buffer_size;
  pc_ = buffer_;
}

AssemblerBase::~AssemblerBase() {
  if (own_buffer_) DeleteArray(buffer_);
}

void AssemblerBase::FlushICache(Isolate* isolate, void* start, size_t size) {
  if (size == 0) return;

#if defined(USE_SIMULATOR)
  base::LockGuard<base::Mutex> lock_guard(isolate->simulator_i_cache_mutex());
  Simulator::FlushICache(isolate->simulator_i_cache(), start, size);
#else
  CpuFeatures::FlushICache(start, size);
#endif  // USE_SIMULATOR
}

void AssemblerBase::Print(Isolate* isolate) {
  OFStream os(stdout);
  v8::internal::Disassembler::Decode(isolate, &os, buffer_, pc_, nullptr);
}

// -----------------------------------------------------------------------------
// Implementation of PredictableCodeSizeScope

PredictableCodeSizeScope::PredictableCodeSizeScope(AssemblerBase* assembler)
    : PredictableCodeSizeScope(assembler, -1) {}

PredictableCodeSizeScope::PredictableCodeSizeScope(AssemblerBase* assembler,
                                                   int expected_size)
    : assembler_(assembler),
      expected_size_(expected_size),
      start_offset_(assembler->pc_offset()),
      old_value_(assembler->predictable_code_size()) {
  assembler_->set_predictable_code_size(true);
}

PredictableCodeSizeScope::~PredictableCodeSizeScope() {
  // TODO(svenpanne) Remove the 'if' when everything works.
  if (expected_size_ >= 0) {
    CHECK_EQ(expected_size_, assembler_->pc_offset() - start_offset_);
  }
  assembler_->set_predictable_code_size(old_value_);
}

// -----------------------------------------------------------------------------
// Implementation of CpuFeatureScope

#ifdef DEBUG
CpuFeatureScope::CpuFeatureScope(AssemblerBase* assembler, CpuFeature f,
                                 CheckPolicy check)
    : assembler_(assembler) {
  DCHECK_IMPLIES(check == kCheckSupported, CpuFeatures::IsSupported(f));
  old_enabled_ = assembler_->enabled_cpu_features();
  assembler_->EnableCpuFeature(f);
}

CpuFeatureScope::~CpuFeatureScope() {
  assembler_->set_enabled_cpu_features(old_enabled_);
}
#endif

bool CpuFeatures::initialized_ = false;
unsigned CpuFeatures::supported_ = 0;
unsigned CpuFeatures::icache_line_size_ = 0;
unsigned CpuFeatures::dcache_line_size_ = 0;

// -----------------------------------------------------------------------------
// Implementation of RelocInfoWriter and RelocIterator
//
// Relocation information is written backwards in memory, from high addresses
// towards low addresses, byte by byte.  Therefore, in the encodings listed
// below, the first byte listed it at the highest address, and successive
// bytes in the record are at progressively lower addresses.
//
// Encoding
//
// The most common modes are given single-byte encodings.  Also, it is
// easy to identify the type of reloc info and skip unwanted modes in
// an iteration.
//
// The encoding relies on the fact that there are fewer than 14
// different relocation modes using standard non-compact encoding.
//
// The first byte of a relocation record has a tag in its low 2 bits:
// Here are the record schemes, depending on the low tag and optional higher
// tags.
//
// Low tag:
//   00: embedded_object:      [6-bit pc delta] 00
//
//   01: code_target:          [6-bit pc delta] 01
//
//   10: short_data_record:    [6-bit pc delta] 10 followed by
//                             [8-bit data delta]
//
//   11: long_record           [6 bit reloc mode] 11
//                             followed by pc delta
//                             followed by optional data depending on type.
//
//  If a pc delta exceeds 6 bits, it is split into a remainder that fits into
//  6 bits and a part that does not. The latter is encoded as a long record
//  with PC_JUMP as pseudo reloc info mode. The former is encoded as part of
//  the following record in the usual way. The long pc jump record has variable
//  length:
//               pc-jump:        [PC_JUMP] 11
//                               [7 bits data] 0
//                                  ...
//                               [7 bits data] 1
//               (Bits 6..31 of pc delta, with leading zeroes
//                dropped, and last non-zero chunk tagged with 1.)

const int kTagBits = 2;
const int kTagMask = (1 << kTagBits) - 1;
const int kLongTagBits = 6;

const int kEmbeddedObjectTag = 0;
const int kCodeTargetTag = 1;
const int kLocatableTag = 2;
const int kDefaultTag = 3;

const int kSmallPCDeltaBits = kBitsPerByte - kTagBits;
const int kSmallPCDeltaMask = (1 << kSmallPCDeltaBits) - 1;
const int RelocInfo::kMaxSmallPCDelta = kSmallPCDeltaMask;

const int kChunkBits = 7;
const int kChunkMask = (1 << kChunkBits) - 1;
const int kLastChunkTagBits = 1;
const int kLastChunkTagMask = 1;
const int kLastChunkTag = 1;

void RelocInfo::set_wasm_context_reference(Isolate* isolate, Address address,
                                           ICacheFlushMode icache_flush_mode) {
  DCHECK(IsWasmContextReference(rmode_));
  set_embedded_address(isolate, address, icache_flush_mode);
}

void RelocInfo::set_global_handle(Isolate* isolate, Address address,
                                  ICacheFlushMode icache_flush_mode) {
  DCHECK_EQ(rmode_, WASM_GLOBAL_HANDLE);
  set_embedded_address(isolate, address, icache_flush_mode);
}

Address RelocInfo::wasm_call_address() const {
  DCHECK_EQ(rmode_, WASM_CALL);
  return Assembler::target_address_at(pc_, constant_pool_);
}

void RelocInfo::set_wasm_call_address(Isolate* isolate, Address address,
                                      ICacheFlushMode icache_flush_mode) {
  DCHECK_EQ(rmode_, WASM_CALL);
  Assembler::set_target_address_at(isolate, pc_, constant_pool_, address,
                                   icache_flush_mode);
}

Address RelocInfo::global_handle() const {
  DCHECK_EQ(rmode_, WASM_GLOBAL_HANDLE);
  return embedded_address();
}

uint32_t RelocInfo::wasm_function_table_size_reference() const {
  DCHECK(IsWasmFunctionTableSizeReference(rmode_));
  return embedded_size();
}

Address RelocInfo::wasm_context_reference() const {
  DCHECK(IsWasmContextReference(rmode_));
  return embedded_address();
}

void RelocInfo::update_wasm_function_table_size_reference(
    Isolate* isolate, uint32_t old_size, uint32_t new_size,
    ICacheFlushMode icache_flush_mode) {
  DCHECK(IsWasmFunctionTableSizeReference(rmode_));
  set_embedded_size(isolate, new_size, icache_flush_mode);
}

void RelocInfo::set_target_address(Isolate* isolate, Address target,
                                   WriteBarrierMode write_barrier_mode,
                                   ICacheFlushMode icache_flush_mode) {
  DCHECK(IsCodeTarget(rmode_) || IsRuntimeEntry(rmode_) || IsWasmCall(rmode_));
  Assembler::set_target_address_at(isolate, pc_, constant_pool_, target,
                                   icache_flush_mode);
  if (write_barrier_mode == UPDATE_WRITE_BARRIER && host() != nullptr &&
      IsCodeTarget(rmode_)) {
    Code* target_code = Code::GetCodeFromTargetAddress(target);
    host()->GetHeap()->incremental_marking()->RecordWriteIntoCode(host(), this,
                                                                  target_code);
  }
}

uint32_t RelocInfoWriter::WriteLongPCJump(uint32_t pc_delta) {
  // Return if the pc_delta can fit in kSmallPCDeltaBits bits.
  // Otherwise write a variable length PC jump for the bits that do
  // not fit in the kSmallPCDeltaBits bits.
  if (is_uintn(pc_delta, kSmallPCDeltaBits)) return pc_delta;
  WriteMode(RelocInfo::PC_JUMP);
  uint32_t pc_jump = pc_delta >> kSmallPCDeltaBits;
  DCHECK_GT(pc_jump, 0);
  // Write kChunkBits size chunks of the pc_jump.
  for (; pc_jump > 0; pc_jump = pc_jump >> kChunkBits) {
    byte b = pc_jump & kChunkMask;
    *--pos_ = b << kLastChunkTagBits;
  }
  // Tag the last chunk so it can be identified.
  *pos_ = *pos_ | kLastChunkTag;
  // Return the remaining kSmallPCDeltaBits of the pc_delta.
  return pc_delta & kSmallPCDeltaMask;
}

void RelocInfoWriter::WriteShortTaggedPC(uint32_t pc_delta, int tag) {
  // Write a byte of tagged pc-delta, possibly preceded by an explicit pc-jump.
  pc_delta = WriteLongPCJump(pc_delta);
  *--pos_ = pc_delta << kTagBits | tag;
}

void RelocInfoWriter::WriteShortData(intptr_t data_delta) {
  *--pos_ = static_cast<byte>(data_delta);
}

void RelocInfoWriter::WriteMode(RelocInfo::Mode rmode) {
  STATIC_ASSERT(RelocInfo::NUMBER_OF_MODES <= (1 << kLongTagBits));
  *--pos_ = static_cast<int>((rmode << kTagBits) | kDefaultTag);
}

void RelocInfoWriter::WriteModeAndPC(uint32_t pc_delta, RelocInfo::Mode rmode) {
  // Write two-byte tagged pc-delta, possibly preceded by var. length pc-jump.
  pc_delta = WriteLongPCJump(pc_delta);
  WriteMode(rmode);
  *--pos_ = pc_delta;
}

void RelocInfoWriter::WriteIntData(int number) {
  for (int i = 0; i < kIntSize; i++) {
    *--pos_ = static_cast<byte>(number);
    // Signed right shift is arithmetic shift.  Tested in test-utils.cc.
    number = number >> kBitsPerByte;
  }
}

void RelocInfoWriter::WriteData(intptr_t data_delta) {
  for (int i = 0; i < kIntptrSize; i++) {
    *--pos_ = static_cast<byte>(data_delta);
    // Signed right shift is arithmetic shift.  Tested in test-utils.cc.
    data_delta = data_delta >> kBitsPerByte;
  }
}

void RelocInfoWriter::Write(const RelocInfo* rinfo) {
  RelocInfo::Mode rmode = rinfo->rmode();
#ifdef DEBUG
  byte* begin_pos = pos_;
#endif
  DCHECK(rinfo->rmode() < RelocInfo::NUMBER_OF_MODES);
  DCHECK_GE(rinfo->pc() - last_pc_, 0);
  // Use unsigned delta-encoding for pc.
  uint32_t pc_delta = static_cast<uint32_t>(rinfo->pc() - last_pc_);

  // The two most common modes are given small tags, and usually fit in a byte.
  if (rmode == RelocInfo::EMBEDDED_OBJECT) {
    WriteShortTaggedPC(pc_delta, kEmbeddedObjectTag);
  } else if (rmode == RelocInfo::CODE_TARGET) {
    WriteShortTaggedPC(pc_delta, kCodeTargetTag);
    DCHECK_LE(begin_pos - pos_, RelocInfo::kMaxCallSize);
  } else if (rmode == RelocInfo::DEOPT_REASON) {
    DCHECK(rinfo->data() < (1 << kBitsPerByte));
    WriteShortTaggedPC(pc_delta, kLocatableTag);
    WriteShortData(rinfo->data());
  } else {
    WriteModeAndPC(pc_delta, rmode);
    if (RelocInfo::IsComment(rmode)) {
      WriteData(rinfo->data());
    } else if (RelocInfo::IsConstPool(rmode) ||
               RelocInfo::IsVeneerPool(rmode) || RelocInfo::IsDeoptId(rmode) ||
               RelocInfo::IsDeoptPosition(rmode)) {
      WriteIntData(static_cast<int>(rinfo->data()));
    }
  }
  last_pc_ = rinfo->pc();
  last_mode_ = rmode;
#ifdef DEBUG
  DCHECK_LE(begin_pos - pos_, kMaxSize);
#endif
}

inline int RelocIterator::AdvanceGetTag() {
  return *--pos_ & kTagMask;
}

inline RelocInfo::Mode RelocIterator::GetMode() {
  return static_cast<RelocInfo::Mode>((*pos_ >> kTagBits) &
                                      ((1 << kLongTagBits) - 1));
}

inline void RelocIterator::ReadShortTaggedPC() {
  rinfo_.pc_ += *pos_ >> kTagBits;
}

inline void RelocIterator::AdvanceReadPC() {
  rinfo_.pc_ += *--pos_;
}

void RelocIterator::AdvanceReadInt() {
  int x = 0;
  for (int i = 0; i < kIntSize; i++) {
    x |= static_cast<int>(*--pos_) << i * kBitsPerByte;
  }
  rinfo_.data_ = x;
}

void RelocIterator::AdvanceReadData() {
  intptr_t x = 0;
  for (int i = 0; i < kIntptrSize; i++) {
    x |= static_cast<intptr_t>(*--pos_) << i * kBitsPerByte;
  }
  rinfo_.data_ = x;
}

void RelocIterator::AdvanceReadLongPCJump() {
  // Read the 32-kSmallPCDeltaBits most significant bits of the
  // pc jump in kChunkBits bit chunks and shift them into place.
  // Stop when the last chunk is encountered.
  uint32_t pc_jump = 0;
  for (int i = 0; i < kIntSize; i++) {
    byte pc_jump_part = *--pos_;
    pc_jump |= (pc_jump_part >> kLastChunkTagBits) << i * kChunkBits;
    if ((pc_jump_part & kLastChunkTagMask) == 1) break;
  }
  // The least significant kSmallPCDeltaBits bits will be added
  // later.
  rinfo_.pc_ += pc_jump << kSmallPCDeltaBits;
}

inline void RelocIterator::ReadShortData() {
  uint8_t unsigned_b = *pos_;
  rinfo_.data_ = unsigned_b;
}

void RelocIterator::next() {
  DCHECK(!done());
  // Basically, do the opposite of RelocInfoWriter::Write.
  // Reading of data is as far as possible avoided for unwanted modes,
  // but we must always update the pc.
  //
  // We exit this loop by returning when we find a mode we want.
  while (pos_ > end_) {
    int tag = AdvanceGetTag();
    if (tag == kEmbeddedObjectTag) {
      ReadShortTaggedPC();
      if (SetMode(RelocInfo::EMBEDDED_OBJECT)) return;
    } else if (tag == kCodeTargetTag) {
      ReadShortTaggedPC();
      if (SetMode(RelocInfo::CODE_TARGET)) return;
    } else if (tag == kLocatableTag) {
      ReadShortTaggedPC();
      Advance();
      if (SetMode(RelocInfo::DEOPT_REASON)) {
        ReadShortData();
        return;
      }
    } else {
      DCHECK_EQ(tag, kDefaultTag);
      RelocInfo::Mode rmode = GetMode();
      if (rmode == RelocInfo::PC_JUMP) {
        AdvanceReadLongPCJump();
      } else {
        AdvanceReadPC();
        if (RelocInfo::IsComment(rmode)) {
          if (SetMode(rmode)) {
            AdvanceReadData();
            return;
          }
          Advance(kIntptrSize);
        } else if (RelocInfo::IsConstPool(rmode) ||
                   RelocInfo::IsVeneerPool(rmode) ||
                   RelocInfo::IsDeoptId(rmode) ||
                   RelocInfo::IsDeoptPosition(rmode)) {
          if (SetMode(rmode)) {
            AdvanceReadInt();
            return;
          }
          Advance(kIntSize);
        } else if (SetMode(static_cast<RelocInfo::Mode>(rmode))) {
          return;
        }
      }
    }
  }
  done_ = true;
}

RelocIterator::RelocIterator(Code* code, int mode_mask) {
  rinfo_.host_ = code;
  rinfo_.pc_ = code->instruction_start();
  rinfo_.data_ = 0;
  rinfo_.constant_pool_ = code->constant_pool();
  // Relocation info is read backwards.
  pos_ = code->relocation_start() + code->relocation_size();
  end_ = code->relocation_start();
  done_ = false;
  mode_mask_ = mode_mask;
  if (mode_mask_ == 0) pos_ = end_;
  next();
}

RelocIterator::RelocIterator(const CodeDesc& desc, int mode_mask) {
  rinfo_.pc_ = desc.buffer;
  rinfo_.data_ = 0;
  // Relocation info is read backwards.
  pos_ = desc.buffer + desc.buffer_size;
  end_ = pos_ - desc.reloc_size;
  done_ = false;
  mode_mask_ = mode_mask;
  if (mode_mask_ == 0) pos_ = end_;
  next();
}

RelocIterator::RelocIterator(Vector<byte> instructions,
                             Vector<const byte> reloc_info, Address const_pool,
                             int mode_mask) {
  rinfo_.pc_ = instructions.start();
  rinfo_.data_ = 0;
  rinfo_.constant_pool_ = const_pool;
  // Relocation info is read backwards.
  pos_ = reloc_info.start() + reloc_info.size();
  end_ = reloc_info.start();
  done_ = false;
  mode_mask_ = mode_mask;
  if (mode_mask_ == 0) pos_ = end_;
  next();
}

// -----------------------------------------------------------------------------
// Implementation of RelocInfo

#ifdef DEBUG
bool RelocInfo::RequiresRelocation(Isolate* isolate, const CodeDesc& desc) {
  // Ensure there are no code targets or embedded objects present in the
  // deoptimization entries, they would require relocation after code
  // generation.
  int mode_mask = RelocInfo::kCodeTargetMask |
                  RelocInfo::ModeMask(RelocInfo::EMBEDDED_OBJECT) |
                  RelocInfo::kApplyMask;
  RelocIterator it(desc, mode_mask);
  return !it.done();
}
#endif

#ifdef ENABLE_DISASSEMBLER
const char* RelocInfo::RelocModeName(RelocInfo::Mode rmode) {
  switch (rmode) {
    case NONE32:
      return "no reloc 32";
    case NONE64:
      return "no reloc 64";
    case EMBEDDED_OBJECT:
      return "embedded object";
    case CODE_TARGET:
      return "code target";
    case RUNTIME_ENTRY:
      return "runtime entry";
    case COMMENT:
      return "comment";
    case EXTERNAL_REFERENCE:
      return "external reference";
    case INTERNAL_REFERENCE:
      return "internal reference";
    case INTERNAL_REFERENCE_ENCODED:
      return "encoded internal reference";
    case DEOPT_SCRIPT_OFFSET:
      return "deopt script offset";
    case DEOPT_INLINING_ID:
      return "deopt inlining id";
    case DEOPT_REASON:
      return "deopt reason";
    case DEOPT_ID:
      return "deopt index";
    case CONST_POOL:
      return "constant pool";
    case VENEER_POOL:
      return "veneer pool";
    case WASM_CONTEXT_REFERENCE:
      return "wasm context reference";
    case WASM_FUNCTION_TABLE_SIZE_REFERENCE:
      return "wasm function table size reference";
    case WASM_GLOBAL_HANDLE:
      return "global handle";
    case WASM_CALL:
      return "internal wasm call";
    case JS_TO_WASM_CALL:
      return "js to wasm call";
    case NUMBER_OF_MODES:
    case PC_JUMP:
      UNREACHABLE();
  }
  return "unknown relocation type";
}

void RelocInfo::Print(Isolate* isolate, std::ostream& os) {  // NOLINT
  os << static_cast<const void*>(pc_) << "  " << RelocModeName(rmode_);
  if (IsComment(rmode_)) {
    os << "  (" << reinterpret_cast<char*>(data_) << ")";
  } else if (rmode_ == DEOPT_SCRIPT_OFFSET || rmode_ == DEOPT_INLINING_ID) {
    os << "  (" << data() << ")";
  } else if (rmode_ == DEOPT_REASON) {
    os << "  ("
       << DeoptimizeReasonToString(static_cast<DeoptimizeReason>(data_)) << ")";
  } else if (rmode_ == EMBEDDED_OBJECT) {
    os << "  (" << Brief(target_object()) << ")";
  } else if (rmode_ == EXTERNAL_REFERENCE) {
    ExternalReferenceEncoder ref_encoder(isolate);
    os << " ("
       << ref_encoder.NameOfAddress(isolate, target_external_reference())
       << ")  (" << static_cast<const void*>(target_external_reference())
       << ")";
  } else if (IsCodeTarget(rmode_)) {
    Code* code = Code::GetCodeFromTargetAddress(target_address());
    os << " (" << Code::Kind2String(code->kind()) << ")  ("
       << static_cast<const void*>(target_address()) << ")";
  } else if (IsRuntimeEntry(rmode_) && isolate->deoptimizer_data() != nullptr) {
    // Depotimization bailouts are stored as runtime entries.
    int id = Deoptimizer::GetDeoptimizationId(
        isolate, target_address(), Deoptimizer::EAGER);
    if (id != Deoptimizer::kNotDeoptimizationEntry) {
      os << "  (deoptimization bailout " << id << ")";
    }
  } else if (IsConstPool(rmode_)) {
    os << " (size " << static_cast<int>(data_) << ")";
  }

  os << "\n";
}
#endif  // ENABLE_DISASSEMBLER

#ifdef VERIFY_HEAP
void RelocInfo::Verify(Isolate* isolate) {
  switch (rmode_) {
    case EMBEDDED_OBJECT:
      Object::VerifyPointer(target_object());
      break;
    case CODE_TARGET: {
      // convert inline target address to code object
      Address addr = target_address();
      CHECK_NOT_NULL(addr);
      // Check that we can find the right code object.
      Code* code = Code::GetCodeFromTargetAddress(addr);
      Object* found = isolate->FindCodeObject(addr);
      CHECK(found->IsCode());
      CHECK(code->address() == HeapObject::cast(found)->address());
      break;
    }
    case INTERNAL_REFERENCE:
    case INTERNAL_REFERENCE_ENCODED: {
      Address target = target_internal_reference();
      Address pc = target_internal_reference_address();
      Code* code = Code::cast(isolate->FindCodeObject(pc));
      CHECK(target >= code->instruction_start());
      CHECK(target <= code->instruction_end());
      break;
    }
    case RUNTIME_ENTRY:
    case COMMENT:
    case EXTERNAL_REFERENCE:
    case DEOPT_SCRIPT_OFFSET:
    case DEOPT_INLINING_ID:
    case DEOPT_REASON:
    case DEOPT_ID:
    case CONST_POOL:
    case VENEER_POOL:
    case WASM_CONTEXT_REFERENCE:
    case WASM_FUNCTION_TABLE_SIZE_REFERENCE:
    case WASM_GLOBAL_HANDLE:
    case WASM_CALL:
    case JS_TO_WASM_CALL:
    case NONE32:
    case NONE64:
      break;
    case NUMBER_OF_MODES:
    case PC_JUMP:
      UNREACHABLE();
      break;
  }
}
#endif  // VERIFY_HEAP

// Implementation of ExternalReference

static ExternalReference::Type BuiltinCallTypeForResultSize(int result_size) {
  switch (result_size) {
    case 1:
      return ExternalReference::BUILTIN_CALL;
    case 2:
      return ExternalReference::BUILTIN_CALL_PAIR;
  }
  UNREACHABLE();
}

void ExternalReference::SetUp() {
  double_constants.min_int = kMinInt;
  double_constants.one_half = 0.5;
  double_constants.minus_one_half = -0.5;
  double_constants.the_hole_nan = kHoleNanInt64;
  double_constants.negative_infinity = -V8_INFINITY;
  double_constants.uint32_bias =
    static_cast<double>(static_cast<uint32_t>(0xFFFFFFFF)) + 1;
}

ExternalReference::ExternalReference(Address address, Isolate* isolate)
    : address_(Redirect(isolate, address)) {}

ExternalReference::ExternalReference(
    ApiFunction* fun, Type type = ExternalReference::BUILTIN_CALL,
    Isolate* isolate = nullptr)
    : address_(Redirect(isolate, fun->address(), type)) {}

ExternalReference::ExternalReference(Runtime::FunctionId id, Isolate* isolate)
    : ExternalReference(Runtime::FunctionForId(id), isolate) {}

ExternalReference::ExternalReference(const Runtime::Function* f,
                                     Isolate* isolate)
    : address_(Redirect(isolate, f->entry,
                        BuiltinCallTypeForResultSize(f->result_size))) {}

ExternalReference ExternalReference::isolate_address(Isolate* isolate) {
  return ExternalReference(isolate);
}

ExternalReference ExternalReference::builtins_address(Isolate* isolate) {
  return ExternalReference(isolate->builtins()->builtins_table_address());
}

ExternalReference ExternalReference::handle_scope_implementer_address(
    Isolate* isolate) {
  return ExternalReference(isolate->handle_scope_implementer_address());
}

ExternalReference ExternalReference::pending_microtask_count_address(
    Isolate* isolate) {
  return ExternalReference(isolate->pending_microtask_count_address());
}

ExternalReference ExternalReference::interpreter_dispatch_table_address(
    Isolate* isolate) {
  return ExternalReference(isolate->interpreter()->dispatch_table_address());
}

ExternalReference ExternalReference::interpreter_dispatch_counters(
    Isolate* isolate) {
  return ExternalReference(
      isolate->interpreter()->bytecode_dispatch_counters_table());
}

ExternalReference ExternalReference::bytecode_size_table_address(
    Isolate* isolate) {
  return ExternalReference(
      interpreter::Bytecodes::bytecode_size_table_address());
}

ExternalReference::ExternalReference(StatsCounter* counter)
  : address_(reinterpret_cast<Address>(counter->GetInternalPointer())) {}

ExternalReference::ExternalReference(IsolateAddressId id, Isolate* isolate)
    : address_(isolate->get_address_from_id(id)) {}

ExternalReference::ExternalReference(const SCTableReference& table_ref)
  : address_(table_ref.address()) {}

ExternalReference ExternalReference::
    incremental_marking_record_write_function(Isolate* isolate) {
  return ExternalReference(Redirect(
      isolate,
      FUNCTION_ADDR(IncrementalMarking::RecordWriteFromCode)));
}

ExternalReference ExternalReference::store_buffer_overflow_function(
    Isolate* isolate) {
  return ExternalReference(Redirect(
      isolate,
      FUNCTION_ADDR(StoreBuffer::StoreBufferOverflow)));
}

ExternalReference ExternalReference::delete_handle_scope_extensions(
    Isolate* isolate) {
  return ExternalReference(Redirect(
      isolate,
      FUNCTION_ADDR(HandleScope::DeleteExtensions)));
}

ExternalReference ExternalReference::get_date_field_function(
    Isolate* isolate) {
  return ExternalReference(Redirect(isolate, FUNCTION_ADDR(JSDate::GetField)));
}

ExternalReference ExternalReference::date_cache_stamp(Isolate* isolate) {
  return ExternalReference(isolate->date_cache()->stamp_address());
}

void ExternalReference::set_redirector(
    Isolate* isolate, ExternalReferenceRedirector* redirector) {
  // We can't stack them.
  DCHECK_NULL(isolate->external_reference_redirector());
  isolate->set_external_reference_redirector(
      reinterpret_cast<ExternalReferenceRedirectorPointer*>(redirector));
}

ExternalReference ExternalReference::stress_deopt_count(Isolate* isolate) {
  return ExternalReference(isolate->stress_deopt_count_address());
}

ExternalReference ExternalReference::force_slow_path(Isolate* isolate) {
  return ExternalReference(isolate->force_slow_path_address());
}

ExternalReference ExternalReference::new_deoptimizer_function(
    Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(Deoptimizer::New)));
}

ExternalReference ExternalReference::compute_output_frames_function(
    Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(Deoptimizer::ComputeOutputFrames)));
}

ExternalReference ExternalReference::wasm_f32_trunc(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::f32_trunc_wrapper)));
}
ExternalReference ExternalReference::wasm_f32_floor(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::f32_floor_wrapper)));
}
ExternalReference ExternalReference::wasm_f32_ceil(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::f32_ceil_wrapper)));
}
ExternalReference ExternalReference::wasm_f32_nearest_int(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::f32_nearest_int_wrapper)));
}

ExternalReference ExternalReference::wasm_f64_trunc(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::f64_trunc_wrapper)));
}

ExternalReference ExternalReference::wasm_f64_floor(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::f64_floor_wrapper)));
}

ExternalReference ExternalReference::wasm_f64_ceil(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::f64_ceil_wrapper)));
}

ExternalReference ExternalReference::wasm_f64_nearest_int(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::f64_nearest_int_wrapper)));
}

ExternalReference ExternalReference::wasm_int64_to_float32(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::int64_to_float32_wrapper)));
}

ExternalReference ExternalReference::wasm_uint64_to_float32(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::uint64_to_float32_wrapper)));
}

ExternalReference ExternalReference::wasm_int64_to_float64(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::int64_to_float64_wrapper)));
}

ExternalReference ExternalReference::wasm_uint64_to_float64(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::uint64_to_float64_wrapper)));
}

ExternalReference ExternalReference::wasm_float32_to_int64(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::float32_to_int64_wrapper)));
}

ExternalReference ExternalReference::wasm_float32_to_uint64(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::float32_to_uint64_wrapper)));
}

ExternalReference ExternalReference::wasm_float64_to_int64(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::float64_to_int64_wrapper)));
}

ExternalReference ExternalReference::wasm_float64_to_uint64(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::float64_to_uint64_wrapper)));
}

ExternalReference ExternalReference::wasm_int64_div(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::int64_div_wrapper)));
}

ExternalReference ExternalReference::wasm_int64_mod(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::int64_mod_wrapper)));
}

ExternalReference ExternalReference::wasm_uint64_div(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::uint64_div_wrapper)));
}

ExternalReference ExternalReference::wasm_uint64_mod(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::uint64_mod_wrapper)));
}

ExternalReference ExternalReference::wasm_word32_ctz(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::word32_ctz_wrapper)));
}

ExternalReference ExternalReference::wasm_word64_ctz(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::word64_ctz_wrapper)));
}

ExternalReference ExternalReference::wasm_word32_popcnt(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::word32_popcnt_wrapper)));
}

ExternalReference ExternalReference::wasm_word64_popcnt(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::word64_popcnt_wrapper)));
}

ExternalReference ExternalReference::wasm_word32_rol(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::word32_rol_wrapper)));
}

ExternalReference ExternalReference::wasm_word32_ror(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::word32_ror_wrapper)));
}

static void f64_acos_wrapper(double* param) {
  WriteDoubleValue(param, base::ieee754::acos(ReadDoubleValue(param)));
}

ExternalReference ExternalReference::f64_acos_wrapper_function(
    Isolate* isolate) {
  return ExternalReference(Redirect(isolate, FUNCTION_ADDR(f64_acos_wrapper)));
}

static void f64_asin_wrapper(double* param) {
  WriteDoubleValue(param, base::ieee754::asin(ReadDoubleValue(param)));
}

ExternalReference ExternalReference::f64_asin_wrapper_function(
    Isolate* isolate) {
  return ExternalReference(Redirect(isolate, FUNCTION_ADDR(f64_asin_wrapper)));
}

ExternalReference ExternalReference::wasm_float64_pow(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::float64_pow_wrapper)));
}

ExternalReference ExternalReference::wasm_set_thread_in_wasm_flag(
    Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::set_thread_in_wasm_flag)));
}

ExternalReference ExternalReference::wasm_clear_thread_in_wasm_flag(
    Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::clear_thread_in_wasm_flag)));
}

static void f64_mod_wrapper(double* param0, double* param1) {
  WriteDoubleValue(param0,
                   Modulo(ReadDoubleValue(param0), ReadDoubleValue(param1)));
}

ExternalReference ExternalReference::f64_mod_wrapper_function(
    Isolate* isolate) {
  return ExternalReference(Redirect(isolate, FUNCTION_ADDR(f64_mod_wrapper)));
}

ExternalReference ExternalReference::wasm_call_trap_callback_for_testing(
    Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(wasm::call_trap_callback_for_testing)));
}

ExternalReference ExternalReference::log_enter_external_function(
    Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(Logger::EnterExternal)));
}

ExternalReference ExternalReference::log_leave_external_function(
    Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(Logger::LeaveExternal)));
}

ExternalReference ExternalReference::roots_array_start(Isolate* isolate) {
  return ExternalReference(isolate->heap()->roots_array_start());
}

ExternalReference ExternalReference::allocation_sites_list_address(
    Isolate* isolate) {
  return ExternalReference(isolate->heap()->allocation_sites_list_address());
}

ExternalReference ExternalReference::address_of_stack_limit(Isolate* isolate) {
  return ExternalReference(isolate->stack_guard()->address_of_jslimit());
}

ExternalReference ExternalReference::address_of_real_stack_limit(
    Isolate* isolate) {
  return ExternalReference(isolate->stack_guard()->address_of_real_jslimit());
}

ExternalReference ExternalReference::address_of_regexp_stack_limit(
    Isolate* isolate) {
  return ExternalReference(isolate->regexp_stack()->limit_address());
}

ExternalReference ExternalReference::store_buffer_top(Isolate* isolate) {
  return ExternalReference(isolate->heap()->store_buffer_top_address());
}

ExternalReference ExternalReference::heap_is_marking_flag_address(
    Isolate* isolate) {
  return ExternalReference(isolate->heap()->IsMarkingFlagAddress());
}

ExternalReference ExternalReference::new_space_allocation_top_address(
    Isolate* isolate) {
  return ExternalReference(isolate->heap()->NewSpaceAllocationTopAddress());
}

ExternalReference ExternalReference::new_space_allocation_limit_address(
    Isolate* isolate) {
  return ExternalReference(isolate->heap()->NewSpaceAllocationLimitAddress());
}

ExternalReference ExternalReference::old_space_allocation_top_address(
    Isolate* isolate) {
  return ExternalReference(isolate->heap()->OldSpaceAllocationTopAddress());
}

ExternalReference ExternalReference::old_space_allocation_limit_address(
    Isolate* isolate) {
  return ExternalReference(isolate->heap()->OldSpaceAllocationLimitAddress());
}

ExternalReference ExternalReference::handle_scope_level_address(
    Isolate* isolate) {
  return ExternalReference(HandleScope::current_level_address(isolate));
}

ExternalReference ExternalReference::handle_scope_next_address(
    Isolate* isolate) {
  return ExternalReference(HandleScope::current_next_address(isolate));
}

ExternalReference ExternalReference::handle_scope_limit_address(
    Isolate* isolate) {
  return ExternalReference(HandleScope::current_limit_address(isolate));
}

ExternalReference ExternalReference::scheduled_exception_address(
    Isolate* isolate) {
  return ExternalReference(isolate->scheduled_exception_address());
}

ExternalReference ExternalReference::address_of_pending_message_obj(
    Isolate* isolate) {
  return ExternalReference(isolate->pending_message_obj_address());
}

ExternalReference ExternalReference::address_of_min_int() {
  return ExternalReference(reinterpret_cast<void*>(&double_constants.min_int));
}

ExternalReference ExternalReference::address_of_one_half() {
  return ExternalReference(reinterpret_cast<void*>(&double_constants.one_half));
}

ExternalReference ExternalReference::address_of_minus_one_half() {
  return ExternalReference(
      reinterpret_cast<void*>(&double_constants.minus_one_half));
}

ExternalReference ExternalReference::address_of_negative_infinity() {
  return ExternalReference(
      reinterpret_cast<void*>(&double_constants.negative_infinity));
}

ExternalReference ExternalReference::address_of_the_hole_nan() {
  return ExternalReference(
      reinterpret_cast<void*>(&double_constants.the_hole_nan));
}

ExternalReference ExternalReference::address_of_uint32_bias() {
  return ExternalReference(
      reinterpret_cast<void*>(&double_constants.uint32_bias));
}

ExternalReference ExternalReference::address_of_float_abs_constant() {
  return ExternalReference(reinterpret_cast<void*>(&float_absolute_constant));
}

ExternalReference ExternalReference::address_of_float_neg_constant() {
  return ExternalReference(reinterpret_cast<void*>(&float_negate_constant));
}

ExternalReference ExternalReference::address_of_double_abs_constant() {
  return ExternalReference(reinterpret_cast<void*>(&double_absolute_constant));
}

ExternalReference ExternalReference::address_of_double_neg_constant() {
  return ExternalReference(reinterpret_cast<void*>(&double_negate_constant));
}

ExternalReference ExternalReference::is_profiling_address(Isolate* isolate) {
  return ExternalReference(isolate->is_profiling_address());
}

ExternalReference ExternalReference::invoke_function_callback(
    Isolate* isolate) {
  Address thunk_address = FUNCTION_ADDR(&InvokeFunctionCallback);
  ExternalReference::Type thunk_type = ExternalReference::PROFILING_API_CALL;
  ApiFunction thunk_fun(thunk_address);
  return ExternalReference(&thunk_fun, thunk_type, isolate);
}

ExternalReference ExternalReference::invoke_accessor_getter_callback(
    Isolate* isolate) {
  Address thunk_address = FUNCTION_ADDR(&InvokeAccessorGetterCallback);
  ExternalReference::Type thunk_type =
      ExternalReference::PROFILING_GETTER_CALL;
  ApiFunction thunk_fun(thunk_address);
  return ExternalReference(&thunk_fun, thunk_type, isolate);
}

#ifndef V8_INTERPRETED_REGEXP

ExternalReference ExternalReference::re_check_stack_guard_state(
    Isolate* isolate) {
  Address function;
#if V8_TARGET_ARCH_X64
  function = FUNCTION_ADDR(RegExpMacroAssemblerX64::CheckStackGuardState);
#elif V8_TARGET_ARCH_IA32
  function = FUNCTION_ADDR(RegExpMacroAssemblerIA32::CheckStackGuardState);
#elif V8_TARGET_ARCH_ARM64
  function = FUNCTION_ADDR(RegExpMacroAssemblerARM64::CheckStackGuardState);
#elif V8_TARGET_ARCH_ARM
  function = FUNCTION_ADDR(RegExpMacroAssemblerARM::CheckStackGuardState);
#elif V8_TARGET_ARCH_PPC
  function = FUNCTION_ADDR(RegExpMacroAssemblerPPC::CheckStackGuardState);
#elif V8_TARGET_ARCH_MIPS
  function = FUNCTION_ADDR(RegExpMacroAssemblerMIPS::CheckStackGuardState);
#elif V8_TARGET_ARCH_MIPS64
  function = FUNCTION_ADDR(RegExpMacroAssemblerMIPS::CheckStackGuardState);
#elif V8_TARGET_ARCH_S390
  function = FUNCTION_ADDR(RegExpMacroAssemblerS390::CheckStackGuardState);
#else
  UNREACHABLE();
#endif
  return ExternalReference(Redirect(isolate, function));
}

ExternalReference ExternalReference::re_grow_stack(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(NativeRegExpMacroAssembler::GrowStack)));
}

ExternalReference ExternalReference::re_case_insensitive_compare_uc16(
    Isolate* isolate) {
  return ExternalReference(Redirect(
      isolate,
      FUNCTION_ADDR(NativeRegExpMacroAssembler::CaseInsensitiveCompareUC16)));
}

ExternalReference ExternalReference::re_word_character_map() {
  return ExternalReference(
      NativeRegExpMacroAssembler::word_character_map_address());
}

ExternalReference ExternalReference::address_of_static_offsets_vector(
    Isolate* isolate) {
  return ExternalReference(
      reinterpret_cast<Address>(isolate->jsregexp_static_offsets_vector()));
}

ExternalReference ExternalReference::address_of_regexp_stack_memory_address(
    Isolate* isolate) {
  return ExternalReference(
      isolate->regexp_stack()->memory_address());
}

ExternalReference ExternalReference::address_of_regexp_stack_memory_size(
    Isolate* isolate) {
  return ExternalReference(isolate->regexp_stack()->memory_size_address());
}

#endif  // V8_INTERPRETED_REGEXP

ExternalReference ExternalReference::ieee754_acos_function(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(base::ieee754::acos), BUILTIN_FP_CALL));
}

ExternalReference ExternalReference::ieee754_acosh_function(Isolate* isolate) {
  return ExternalReference(Redirect(
      isolate, FUNCTION_ADDR(base::ieee754::acosh), BUILTIN_FP_FP_CALL));
}

ExternalReference ExternalReference::ieee754_asin_function(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(base::ieee754::asin), BUILTIN_FP_CALL));
}

ExternalReference ExternalReference::ieee754_asinh_function(Isolate* isolate) {
  return ExternalReference(Redirect(
      isolate, FUNCTION_ADDR(base::ieee754::asinh), BUILTIN_FP_FP_CALL));
}

ExternalReference ExternalReference::ieee754_atan_function(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(base::ieee754::atan), BUILTIN_FP_CALL));
}

ExternalReference ExternalReference::ieee754_atanh_function(Isolate* isolate) {
  return ExternalReference(Redirect(
      isolate, FUNCTION_ADDR(base::ieee754::atanh), BUILTIN_FP_FP_CALL));
}

ExternalReference ExternalReference::ieee754_atan2_function(Isolate* isolate) {
  return ExternalReference(Redirect(
      isolate, FUNCTION_ADDR(base::ieee754::atan2), BUILTIN_FP_FP_CALL));
}

ExternalReference ExternalReference::ieee754_cbrt_function(Isolate* isolate) {
  return ExternalReference(Redirect(isolate, FUNCTION_ADDR(base::ieee754::cbrt),
                                    BUILTIN_FP_FP_CALL));
}

ExternalReference ExternalReference::ieee754_cos_function(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(base::ieee754::cos), BUILTIN_FP_CALL));
}

ExternalReference ExternalReference::ieee754_cosh_function(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(base::ieee754::cosh), BUILTIN_FP_CALL));
}

ExternalReference ExternalReference::ieee754_exp_function(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(base::ieee754::exp), BUILTIN_FP_CALL));
}

ExternalReference ExternalReference::ieee754_expm1_function(Isolate* isolate) {
  return ExternalReference(Redirect(
      isolate, FUNCTION_ADDR(base::ieee754::expm1), BUILTIN_FP_FP_CALL));
}

ExternalReference ExternalReference::ieee754_log_function(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(base::ieee754::log), BUILTIN_FP_CALL));
}

ExternalReference ExternalReference::ieee754_log1p_function(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(base::ieee754::log1p), BUILTIN_FP_CALL));
}

ExternalReference ExternalReference::ieee754_log10_function(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(base::ieee754::log10), BUILTIN_FP_CALL));
}

ExternalReference ExternalReference::ieee754_log2_function(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(base::ieee754::log2), BUILTIN_FP_CALL));
}

ExternalReference ExternalReference::ieee754_sin_function(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(base::ieee754::sin), BUILTIN_FP_CALL));
}

ExternalReference ExternalReference::ieee754_sinh_function(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(base::ieee754::sinh), BUILTIN_FP_CALL));
}

ExternalReference ExternalReference::ieee754_tan_function(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(base::ieee754::tan), BUILTIN_FP_CALL));
}

ExternalReference ExternalReference::ieee754_tanh_function(Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(base::ieee754::tanh), BUILTIN_FP_CALL));
}

void* libc_memchr(void* string, int character, size_t search_length) {
  return memchr(string, character, search_length);
}

ExternalReference ExternalReference::libc_memchr_function(Isolate* isolate) {
  return ExternalReference(Redirect(isolate, FUNCTION_ADDR(libc_memchr)));
}

void* libc_memcpy(void* dest, const void* src, size_t n) {
  return memcpy(dest, src, n);
}

ExternalReference ExternalReference::libc_memcpy_function(Isolate* isolate) {
  return ExternalReference(Redirect(isolate, FUNCTION_ADDR(libc_memcpy)));
}

void* libc_memmove(void* dest, const void* src, size_t n) {
  return memmove(dest, src, n);
}

ExternalReference ExternalReference::libc_memmove_function(Isolate* isolate) {
  return ExternalReference(Redirect(isolate, FUNCTION_ADDR(libc_memmove)));
}

void* libc_memset(void* dest, int byte, size_t n) {
  DCHECK_EQ(static_cast<char>(byte), byte);
  return memset(dest, byte, n);
}

ExternalReference ExternalReference::libc_memset_function(Isolate* isolate) {
  return ExternalReference(Redirect(isolate, FUNCTION_ADDR(libc_memset)));
}

#if V8_OS_STARBOARD
namespace {
int no_printf(const char *format, ...) {
  return 0;
}
}
ExternalReference ExternalReference::printf_function(Isolate* isolate) {
  return ExternalReference(Redirect(isolate, FUNCTION_ADDR(no_printf)));
}
#else
ExternalReference ExternalReference::printf_function(Isolate* isolate) {
  return ExternalReference(Redirect(isolate, FUNCTION_ADDR(std::printf)));
}
#endif

template <typename SubjectChar, typename PatternChar>
ExternalReference ExternalReference::search_string_raw(Isolate* isolate) {
  auto f = SearchStringRaw<SubjectChar, PatternChar>;
  return ExternalReference(Redirect(isolate, FUNCTION_ADDR(f)));
}

ExternalReference ExternalReference::orderedhashmap_gethash_raw(
    Isolate* isolate) {
  auto f = OrderedHashMap::GetHash;
  return ExternalReference(Redirect(isolate, FUNCTION_ADDR(f)));
}

ExternalReference ExternalReference::get_or_create_hash_raw(Isolate* isolate) {
  typedef Smi* (*GetOrCreateHash)(Isolate * isolate, Object * key);
  GetOrCreateHash f = Object::GetOrCreateHash;
  return ExternalReference(Redirect(isolate, FUNCTION_ADDR(f)));
}

ExternalReference ExternalReference::jsreceiver_create_identity_hash(
    Isolate* isolate) {
  typedef Smi* (*CreateIdentityHash)(Isolate * isolate, JSReceiver * key);
  CreateIdentityHash f = JSReceiver::CreateIdentityHash;
  return ExternalReference(Redirect(isolate, FUNCTION_ADDR(f)));
}

ExternalReference
ExternalReference::copy_fast_number_jsarray_elements_to_typed_array(
    Isolate* isolate) {
  return ExternalReference(Redirect(
      isolate, FUNCTION_ADDR(CopyFastNumberJSArrayElementsToTypedArray)));
}

ExternalReference ExternalReference::copy_typed_array_elements_to_typed_array(
    Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(CopyTypedArrayElementsToTypedArray)));
}

ExternalReference ExternalReference::try_internalize_string_function(
    Isolate* isolate) {
  return ExternalReference(Redirect(
      isolate, FUNCTION_ADDR(StringTable::LookupStringIfExists_NoAllocate)));
}

ExternalReference ExternalReference::check_object_type(Isolate* isolate) {
  return ExternalReference(Redirect(isolate, FUNCTION_ADDR(CheckObjectType)));
}

#ifdef V8_INTL_SUPPORT
ExternalReference ExternalReference::intl_convert_one_byte_to_lower(
    Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(ConvertOneByteToLower)));
}

ExternalReference ExternalReference::intl_to_latin1_lower_table(
    Isolate* isolate) {
  uint8_t* ptr = const_cast<uint8_t*>(ToLatin1LowerTable());
  return ExternalReference(reinterpret_cast<Address>(ptr));
}
#endif  // V8_INTL_SUPPORT

// Explicit instantiations for all combinations of 1- and 2-byte strings.
template ExternalReference
ExternalReference::search_string_raw<const uint8_t, const uint8_t>(Isolate*);
template ExternalReference
ExternalReference::search_string_raw<const uint8_t, const uc16>(Isolate*);
template ExternalReference
ExternalReference::search_string_raw<const uc16, const uint8_t>(Isolate*);
template ExternalReference
ExternalReference::search_string_raw<const uc16, const uc16>(Isolate*);

ExternalReference ExternalReference::page_flags(Page* page) {
  return ExternalReference(reinterpret_cast<Address>(page) +
                           MemoryChunk::kFlagsOffset);
}

ExternalReference ExternalReference::ForDeoptEntry(Address entry) {
  return ExternalReference(entry);
}

ExternalReference ExternalReference::cpu_features() {
  DCHECK(CpuFeatures::initialized_);
  return ExternalReference(&CpuFeatures::supported_);
}

ExternalReference ExternalReference::promise_hook_or_debug_is_active_address(
    Isolate* isolate) {
  return ExternalReference(isolate->promise_hook_or_debug_is_active_address());
}

ExternalReference ExternalReference::debug_is_active_address(
    Isolate* isolate) {
  return ExternalReference(isolate->debug()->is_active_address());
}

ExternalReference ExternalReference::debug_hook_on_function_call_address(
    Isolate* isolate) {
  return ExternalReference(isolate->debug()->hook_on_function_call_address());
}

ExternalReference ExternalReference::runtime_function_table_address(
    Isolate* isolate) {
  return ExternalReference(
      const_cast<Runtime::Function*>(Runtime::RuntimeFunctionTable(isolate)));
}

ExternalReference ExternalReference::invalidate_prototype_chains_function(
    Isolate* isolate) {
  return ExternalReference(
      Redirect(isolate, FUNCTION_ADDR(JSObject::InvalidatePrototypeChains)));
}

double power_helper(Isolate* isolate, double x, double y) {
  int y_int = static_cast<int>(y);
  if (y == y_int) {
    return power_double_int(x, y_int);  // Returns 1 if exponent is 0.
  }
  if (y == 0.5) {
    lazily_initialize_fast_sqrt(isolate);
    return (std::isinf(x)) ? V8_INFINITY
                           : fast_sqrt(x + 0.0, isolate);  // Convert -0 to +0.
  }
  if (y == -0.5) {
    lazily_initialize_fast_sqrt(isolate);
    return (std::isinf(x)) ? 0 : 1.0 / fast_sqrt(x + 0.0,
                                                 isolate);  // Convert -0 to +0.
  }
  return power_double_double(x, y);
}

// Helper function to compute x^y, where y is known to be an
// integer. Uses binary decomposition to limit the number of
// multiplications; see the discussion in "Hacker's Delight" by Henry
// S. Warren, Jr., figure 11-6, page 213.
double power_double_int(double x, int y) {
  double m = (y < 0) ? 1 / x : x;
  unsigned n = (y < 0) ? -y : y;
  double p = 1;
  while (n != 0) {
    if ((n & 1) != 0) p *= m;
    m *= m;
    if ((n & 2) != 0) p *= m;
    m *= m;
    n >>= 2;
  }
  return p;
}

double power_double_double(double x, double y) {
  // The checks for special cases can be dropped in ia32 because it has already
  // been done in generated code before bailing out here.
  if (std::isnan(y) || ((x == 1 || x == -1) && std::isinf(y))) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return Pow(x, y);
}

double modulo_double_double(double x, double y) { return Modulo(x, y); }

ExternalReference ExternalReference::power_double_double_function(
    Isolate* isolate) {
  return ExternalReference(Redirect(isolate,
                                    FUNCTION_ADDR(power_double_double),
                                    BUILTIN_FP_FP_CALL));
}

ExternalReference ExternalReference::mod_two_doubles_operation(
    Isolate* isolate) {
  return ExternalReference(Redirect(
      isolate, FUNCTION_ADDR(modulo_double_double), BUILTIN_FP_FP_CALL));
}

ExternalReference ExternalReference::debug_last_step_action_address(
    Isolate* isolate) {
  return ExternalReference(isolate->debug()->last_step_action_address());
}

ExternalReference ExternalReference::debug_suspended_generator_address(
    Isolate* isolate) {
  return ExternalReference(isolate->debug()->suspended_generator_address());
}

ExternalReference ExternalReference::debug_restart_fp_address(
    Isolate* isolate) {
  return ExternalReference(isolate->debug()->restart_fp_address());
}

ExternalReference ExternalReference::fixed_typed_array_base_data_offset() {
  return ExternalReference(reinterpret_cast<void*>(
      FixedTypedArrayBase::kDataOffset - kHeapObjectTag));
}

bool operator==(ExternalReference lhs, ExternalReference rhs) {
  return lhs.address() == rhs.address();
}

bool operator!=(ExternalReference lhs, ExternalReference rhs) {
  return !(lhs == rhs);
}

size_t hash_value(ExternalReference reference) {
  return base::hash<Address>()(reference.address());
}

std::ostream& operator<<(std::ostream& os, ExternalReference reference) {
  os << static_cast<const void*>(reference.address());
  const Runtime::Function* fn = Runtime::FunctionForEntry(reference.address());
  if (fn) os << "<" << fn->name << ".entry>";
  return os;
}

ConstantPoolBuilder::ConstantPoolBuilder(int ptr_reach_bits,
                                         int double_reach_bits) {
  info_[ConstantPoolEntry::INTPTR].entries.reserve(64);
  info_[ConstantPoolEntry::INTPTR].regular_reach_bits = ptr_reach_bits;
  info_[ConstantPoolEntry::DOUBLE].regular_reach_bits = double_reach_bits;
}

ConstantPoolEntry::Access ConstantPoolBuilder::NextAccess(
    ConstantPoolEntry::Type type) const {
  const PerTypeEntryInfo& info = info_[type];

  if (info.overflow()) return ConstantPoolEntry::OVERFLOWED;

  int dbl_count = info_[ConstantPoolEntry::DOUBLE].regular_count;
  int dbl_offset = dbl_count * kDoubleSize;
  int ptr_count = info_[ConstantPoolEntry::INTPTR].regular_count;
  int ptr_offset = ptr_count * kPointerSize + dbl_offset;

  if (type == ConstantPoolEntry::DOUBLE) {
    // Double overflow detection must take into account the reach for both types
    int ptr_reach_bits = info_[ConstantPoolEntry::INTPTR].regular_reach_bits;
    if (!is_uintn(dbl_offset, info.regular_reach_bits) ||
        (ptr_count > 0 &&
         !is_uintn(ptr_offset + kDoubleSize - kPointerSize, ptr_reach_bits))) {
      return ConstantPoolEntry::OVERFLOWED;
    }
  } else {
    DCHECK(type == ConstantPoolEntry::INTPTR);
    if (!is_uintn(ptr_offset, info.regular_reach_bits)) {
      return ConstantPoolEntry::OVERFLOWED;
    }
  }

  return ConstantPoolEntry::REGULAR;
}

ConstantPoolEntry::Access ConstantPoolBuilder::AddEntry(
    ConstantPoolEntry& entry, ConstantPoolEntry::Type type) {
  DCHECK(!emitted_label_.is_bound());
  PerTypeEntryInfo& info = info_[type];
  const int entry_size = ConstantPoolEntry::size(type);
  bool merged = false;

  if (entry.sharing_ok()) {
    // Try to merge entries
    std::vector<ConstantPoolEntry>::iterator it = info.shared_entries.begin();
    int end = static_cast<int>(info.shared_entries.size());
    for (int i = 0; i < end; i++, it++) {
      if ((entry_size == kPointerSize) ? entry.value() == it->value()
                                       : entry.value64() == it->value64()) {
        // Merge with found entry.
        entry.set_merged_index(i);
        merged = true;
        break;
      }
    }
  }

  // By definition, merged entries have regular access.
  DCHECK(!merged || entry.merged_index() < info.regular_count);
  ConstantPoolEntry::Access access =
      (merged ? ConstantPoolEntry::REGULAR : NextAccess(type));

  // Enforce an upper bound on search time by limiting the search to
  // unique sharable entries which fit in the regular section.
  if (entry.sharing_ok() && !merged && access == ConstantPoolEntry::REGULAR) {
    info.shared_entries.push_back(entry);
  } else {
    info.entries.push_back(entry);
  }

  // We're done if we found a match or have already triggered the
  // overflow state.
  if (merged || info.overflow()) return access;

  if (access == ConstantPoolEntry::REGULAR) {
    info.regular_count++;
  } else {
    info.overflow_start = static_cast<int>(info.entries.size()) - 1;
  }

  return access;
}

void ConstantPoolBuilder::EmitSharedEntries(Assembler* assm,
                                            ConstantPoolEntry::Type type) {
  PerTypeEntryInfo& info = info_[type];
  std::vector<ConstantPoolEntry>& shared_entries = info.shared_entries;
  const int entry_size = ConstantPoolEntry::size(type);
  int base = emitted_label_.pos();
  DCHECK_GT(base, 0);
  int shared_end = static_cast<int>(shared_entries.size());
  std::vector<ConstantPoolEntry>::iterator shared_it = shared_entries.begin();
  for (int i = 0; i < shared_end; i++, shared_it++) {
    int offset = assm->pc_offset() - base;
    shared_it->set_offset(offset);  // Save offset for merged entries.
    if (entry_size == kPointerSize) {
      assm->dp(shared_it->value());
    } else {
      assm->dq(shared_it->value64());
    }
    DCHECK(is_uintn(offset, info.regular_reach_bits));

    // Patch load sequence with correct offset.
    assm->PatchConstantPoolAccessInstruction(shared_it->position(), offset,
                                             ConstantPoolEntry::REGULAR, type);
  }
}

void ConstantPoolBuilder::EmitGroup(Assembler* assm,
                                    ConstantPoolEntry::Access access,
                                    ConstantPoolEntry::Type type) {
  PerTypeEntryInfo& info = info_[type];
  const bool overflow = info.overflow();
  std::vector<ConstantPoolEntry>& entries = info.entries;
  std::vector<ConstantPoolEntry>& shared_entries = info.shared_entries;
  const int entry_size = ConstantPoolEntry::size(type);
  int base = emitted_label_.pos();
  DCHECK_GT(base, 0);
  int begin;
  int end;

  if (access == ConstantPoolEntry::REGULAR) {
    // Emit any shared entries first
    EmitSharedEntries(assm, type);
  }

  if (access == ConstantPoolEntry::REGULAR) {
    begin = 0;
    end = overflow ? info.overflow_start : static_cast<int>(entries.size());
  } else {
    DCHECK(access == ConstantPoolEntry::OVERFLOWED);
    if (!overflow) return;
    begin = info.overflow_start;
    end = static_cast<int>(entries.size());
  }

  std::vector<ConstantPoolEntry>::iterator it = entries.begin();
  if (begin > 0) std::advance(it, begin);
  for (int i = begin; i < end; i++, it++) {
    // Update constant pool if necessary and get the entry's offset.
    int offset;
    ConstantPoolEntry::Access entry_access;
    if (!it->is_merged()) {
      // Emit new entry
      offset = assm->pc_offset() - base;
      entry_access = access;
      if (entry_size == kPointerSize) {
        assm->dp(it->value());
      } else {
        assm->dq(it->value64());
      }
    } else {
      // Retrieve offset from shared entry.
      offset = shared_entries[it->merged_index()].offset();
      entry_access = ConstantPoolEntry::REGULAR;
    }

    DCHECK(entry_access == ConstantPoolEntry::OVERFLOWED ||
           is_uintn(offset, info.regular_reach_bits));

    // Patch load sequence with correct offset.
    assm->PatchConstantPoolAccessInstruction(it->position(), offset,
                                             entry_access, type);
  }
}

// Emit and return position of pool.  Zero implies no constant pool.
int ConstantPoolBuilder::Emit(Assembler* assm) {
  bool emitted = emitted_label_.is_bound();
  bool empty = IsEmpty();

  if (!emitted) {
    // Mark start of constant pool.  Align if necessary.
    if (!empty) assm->DataAlign(kDoubleSize);
    assm->bind(&emitted_label_);
    if (!empty) {
      // Emit in groups based on access and type.
      // Emit doubles first for alignment purposes.
      EmitGroup(assm, ConstantPoolEntry::REGULAR, ConstantPoolEntry::DOUBLE);
      EmitGroup(assm, ConstantPoolEntry::REGULAR, ConstantPoolEntry::INTPTR);
      if (info_[ConstantPoolEntry::DOUBLE].overflow()) {
        assm->DataAlign(kDoubleSize);
        EmitGroup(assm, ConstantPoolEntry::OVERFLOWED,
                  ConstantPoolEntry::DOUBLE);
      }
      if (info_[ConstantPoolEntry::INTPTR].overflow()) {
        EmitGroup(assm, ConstantPoolEntry::OVERFLOWED,
                  ConstantPoolEntry::INTPTR);
      }
    }
  }

  return !empty ? emitted_label_.pos() : 0;
}

HeapObjectRequest::HeapObjectRequest(double heap_number, int offset)
    : kind_(kHeapNumber), offset_(offset) {
  value_.heap_number = heap_number;
  DCHECK(!IsSmiDouble(value_.heap_number));
}

HeapObjectRequest::HeapObjectRequest(CodeStub* code_stub, int offset)
    : kind_(kCodeStub), offset_(offset) {
  value_.code_stub = code_stub;
  DCHECK_NOT_NULL(value_.code_stub);
}

// Platform specific but identical code for all the platforms.

void Assembler::RecordDeoptReason(DeoptimizeReason reason,
                                  SourcePosition position, int id) {
  EnsureSpace ensure_space(this);
  RecordRelocInfo(RelocInfo::DEOPT_SCRIPT_OFFSET, position.ScriptOffset());
  RecordRelocInfo(RelocInfo::DEOPT_INLINING_ID, position.InliningId());
  RecordRelocInfo(RelocInfo::DEOPT_REASON, static_cast<int>(reason));
  RecordRelocInfo(RelocInfo::DEOPT_ID, id);
}

void Assembler::RecordComment(const char* msg) {
  if (FLAG_code_comments) {
    EnsureSpace ensure_space(this);
    RecordRelocInfo(RelocInfo::COMMENT, reinterpret_cast<intptr_t>(msg));
  }
}

void Assembler::DataAlign(int m) {
  DCHECK(m >= 2 && base::bits::IsPowerOfTwo(m));
  while ((pc_offset() & (m - 1)) != 0) {
    db(0);
  }
}

void Assembler::RequestHeapObject(HeapObjectRequest request) {
  request.set_offset(pc_offset());
  heap_object_requests_.push_front(request);
}

namespace {
int caller_saved_codes[kNumJSCallerSaved];
}

void SetUpJSCallerSavedCodeData() {
  int i = 0;
  for (int r = 0; r < kNumRegs; r++)
    if ((kJSCallerSaved & (1 << r)) != 0) caller_saved_codes[i++] = r;

  DCHECK_EQ(i, kNumJSCallerSaved);
}

int JSCallerSavedCode(int n) {
  DCHECK(0 <= n && n < kNumJSCallerSaved);
  return caller_saved_codes[n];
}

}  // namespace internal
}  // namespace v8
