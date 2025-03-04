# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
{
  'variables': {
    'mozjs-45_sources': [
      'js/src/asmjs/AsmJSFrameIterator.cpp',
      'js/src/asmjs/AsmJSLink.cpp',
      'js/src/asmjs/AsmJSModule.cpp',
      'js/src/asmjs/AsmJSSignalHandlersStarboard.cpp',
      'js/src/asmjs/AsmJSValidate.cpp',
      'js/src/asmjs/WasmGenerator.cpp',
      'js/src/asmjs/WasmIonCompile.cpp',
      'js/src/asmjs/WasmStubs.cpp',
      'js/src/builtin/AtomicsObject.cpp',
      'js/src/builtin/Eval.cpp',
      'js/src/builtin/Intl.cpp',
      'js/src/builtin/MapObject.cpp',
      'js/src/builtin/ModuleObject.cpp',
      'js/src/builtin/Object.cpp',
      'js/src/builtin/Profilers.cpp',
      'js/src/builtin/Reflect.cpp',
      'js/src/builtin/ReflectParse.cpp',
      'js/src/builtin/RegExp.cpp',
      'js/src/builtin/SIMD.cpp',
      'js/src/builtin/SymbolObject.cpp',
      'js/src/builtin/TestingFunctions.cpp',
      'js/src/builtin/TypedObject.cpp',
      'js/src/builtin/WeakMapObject.cpp',
      'js/src/builtin/WeakSetObject.cpp',
      'js/src/ds/LifoAlloc.cpp',
      'js/src/frontend/BytecodeCompiler.cpp',
      'js/src/frontend/BytecodeEmitter.cpp',
      'js/src/frontend/FoldConstants.cpp',
      'js/src/frontend/NameFunctions.cpp',
      'js/src/frontend/ParseMaps.cpp',
      'js/src/frontend/ParseNode.cpp',
      'js/src/frontend/Parser.cpp',
      'js/src/frontend/TokenStream.cpp',
      'js/src/gc/Allocator.cpp',
      'js/src/gc/Barrier.cpp',
      'js/src/gc/GCTrace.cpp',
      'js/src/gc/Iteration.cpp',
      'js/src/gc/Marking.cpp',
      'js/src/gc/Memory.cpp',
      'js/src/gc/MemoryProfiler.cpp',
      'js/src/gc/Nursery.cpp',
      'js/src/gc/RootMarking.cpp',
      'js/src/gc/Statistics.cpp',
      'js/src/gc/StoreBuffer.cpp',
      'js/src/gc/Tracer.cpp',
      'js/src/gc/Verifier.cpp',
      'js/src/gc/Zone.cpp',
      'js/src/irregexp/NativeRegExpMacroAssembler.cpp',
      'js/src/irregexp/RegExpAST.cpp',
      'js/src/irregexp/RegExpEngine.cpp',
      'js/src/irregexp/RegExpInterpreter.cpp',
      'js/src/irregexp/RegExpMacroAssembler.cpp',
      'js/src/irregexp/RegExpParser.cpp',
      'js/src/irregexp/RegExpStack.cpp',
      'js/src/jit/AliasAnalysis.cpp',
      'js/src/jit/AlignmentMaskAnalysis.cpp',
      'js/src/jit/BacktrackingAllocator.cpp',
      'js/src/jit/Bailouts.cpp',
      'js/src/jit/BaselineBailouts.cpp',
      'js/src/jit/BaselineCompiler.cpp',
      'js/src/jit/BaselineDebugModeOSR.cpp',
      'js/src/jit/BaselineFrame.cpp',
      'js/src/jit/BaselineFrameInfo.cpp',
      'js/src/jit/BaselineIC.cpp',
      'js/src/jit/BaselineInspector.cpp',
      'js/src/jit/BaselineJIT.cpp',
      'js/src/jit/BitSet.cpp',
      'js/src/jit/BytecodeAnalysis.cpp',
      'js/src/jit/C1Spewer.cpp',
      'js/src/jit/CodeGenerator.cpp',
      'js/src/jit/CompileWrappers.cpp',
      'js/src/jit/Disassembler.cpp',
      'js/src/jit/EagerSimdUnbox.cpp',
      'js/src/jit/EdgeCaseAnalysis.cpp',
      'js/src/jit/EffectiveAddressAnalysis.cpp',
      'js/src/jit/ExecutableAllocator.cpp',
      'js/src/jit/ExecutableAllocatorStarboard.cpp',
      'js/src/jit/InstructionReordering.cpp',
      'js/src/jit/Ion.cpp',
      'js/src/jit/IonAnalysis.cpp',
      'js/src/jit/IonBuilder.cpp',
      'js/src/jit/IonCaches.cpp',
      'js/src/jit/IonOptimizationLevels.cpp',
      'js/src/jit/JitcodeMap.cpp',
      'js/src/jit/JitFrames.cpp',
      'js/src/jit/JitOptions.cpp',
      'js/src/jit/JitSpewer.cpp',
      'js/src/jit/JSONSpewer.cpp',
      'js/src/jit/LICM.cpp',
      'js/src/jit/LIR.cpp',
      'js/src/jit/LoopUnroller.cpp',
      'js/src/jit/Lowering.cpp',
      'js/src/jit/MacroAssembler.cpp',
      'js/src/jit/MCallOptimize.cpp',
      'js/src/jit/MIR.cpp',
      'js/src/jit/MIRGraph.cpp',
      'js/src/jit/MoveResolver.cpp',
      'js/src/jit/OptimizationTracking.cpp',
      'js/src/jit/PerfSpewer.cpp',
      'js/src/jit/RangeAnalysis.cpp',
      'js/src/jit/Recover.cpp',
      'js/src/jit/RegisterAllocator.cpp',
      'js/src/jit/RematerializedFrame.cpp',
      'js/src/jit/Safepoints.cpp',
      'js/src/jit/ScalarReplacement.cpp',
      'js/src/jit/shared/BaselineCompiler-shared.cpp',
      'js/src/jit/shared/CodeGenerator-shared.cpp',
      'js/src/jit/shared/Lowering-shared.cpp',
      'js/src/jit/SharedIC.cpp',
      'js/src/jit/Sink.cpp',
      'js/src/jit/Snapshots.cpp',
      'js/src/jit/StupidAllocator.cpp',
      'js/src/jit/TypedObjectPrediction.cpp',
      'js/src/jit/TypePolicy.cpp',
      'js/src/jit/ValueNumbering.cpp',
      'js/src/jit/VMFunctions.cpp',
      'js/src/jsalloc.cpp',
      'js/src/jsapi.cpp',
      'js/src/jsarray.cpp',
      'js/src/jsatom.cpp',
      'js/src/jsbool.cpp',
      'js/src/jscntxt.cpp',
      'js/src/jscompartment.cpp',
      'js/src/jsdate.cpp',
      'js/src/jsdtoa.cpp',
      'js/src/jsexn.cpp',
      'js/src/jsfriendapi.cpp',
      'js/src/jsfun.cpp',
      'js/src/jsgc.cpp',
      'js/src/jsiter.cpp',
      'js/src/jsmath.cpp',
      'js/src/jsnativestack.cpp',
      'js/src/jsnum.cpp',
      'js/src/jsobj.cpp',
      'js/src/json.cpp',
      'js/src/jsopcode.cpp',
      'js/src/jsprf.cpp',
      'js/src/jspropertytree.cpp',
      'js/src/jsscript.cpp',
      'js/src/jsstr.cpp',
      'js/src/jsutil.cpp',
      'js/src/jswatchpoint.cpp',
      'js/src/jsweakmap.cpp',
      'js/src/memory_allocator_reporter.cpp',
      'js/src/perf/jsperf.cpp',
      'js/src/perf/pm_stub.cpp',
      'js/src/proxy/BaseProxyHandler.cpp',
      'js/src/proxy/CrossCompartmentWrapper.cpp',
      'js/src/proxy/DeadObjectProxy.cpp',
      'js/src/proxy/DirectProxyHandler.cpp',
      'js/src/proxy/OpaqueCrossCompartmentWrapper.cpp',
      'js/src/proxy/Proxy.cpp',
      'js/src/proxy/ScriptedDirectProxyHandler.cpp',
      'js/src/proxy/ScriptedIndirectProxyHandler.cpp',
      'js/src/proxy/SecurityWrapper.cpp',
      'js/src/proxy/Wrapper.cpp',
      'js/src/vm/ArgumentsObject.cpp',
      'js/src/vm/ArrayBufferObject.cpp',
      'js/src/vm/CallNonGenericMethod.cpp',
      'js/src/vm/CharacterEncoding.cpp',
      'js/src/vm/CodeCoverage.cpp',
      'js/src/vm/Compression.cpp',
      'js/src/vm/DateTime.cpp',
      'js/src/vm/Debugger.cpp',
      'js/src/vm/DebuggerMemory.cpp',
      'js/src/vm/ErrorObject.cpp',
      'js/src/vm/ForOfIterator.cpp',
      'js/src/vm/GeneratorObject.cpp',
      'js/src/vm/GlobalObject.cpp',
      'js/src/vm/HelperThreads.cpp',
      'js/src/vm/Id.cpp',
      'js/src/vm/Initialization.cpp',
      'js/src/vm/Interpreter.cpp',
      'js/src/vm/JSONParser.cpp',
      'js/src/vm/MemoryMetrics.cpp',
      'js/src/vm/Monitor.cpp',
      'js/src/vm/NativeObject.cpp',
      'js/src/vm/ObjectGroup.cpp',
      'js/src/vm/PIC.cpp',
      'js/src/vm/PosixNSPR.cpp',
      'js/src/vm/Printer.cpp',
      'js/src/vm/Probes.cpp',
      'js/src/vm/ProxyObject.cpp',
      'js/src/vm/ReceiverGuard.cpp',
      'js/src/vm/RegExpObject.cpp',
      'js/src/vm/RegExpStatics.cpp',
      'js/src/vm/Runtime.cpp',
      'js/src/vm/SavedStacks.cpp',
      'js/src/vm/ScopeObject.cpp',
      'js/src/vm/SelfHosting.cpp',
      'js/src/vm/Shape.cpp',
      'js/src/vm/SharedArrayObject.cpp',
      'js/src/vm/SPSProfiler.cpp',
      'js/src/vm/Stack.cpp',
      'js/src/vm/Stopwatch.cpp',
      'js/src/vm/String.cpp',
      'js/src/vm/StringBuffer.cpp',
      'js/src/vm/StructuredClone.cpp',
      'js/src/vm/Symbol.cpp',
      'js/src/vm/TaggedProto.cpp',
      'js/src/vm/Time.cpp',
      'js/src/vm/TraceLogging.cpp',
      'js/src/vm/TraceLoggingGraph.cpp',
      'js/src/vm/TraceLoggingTypes.cpp',
      'js/src/vm/TypedArrayObject.cpp',
      'js/src/vm/TypeInference.cpp',
      'js/src/vm/UbiNode.cpp',
      'js/src/vm/UbiNodeCensus.cpp',
      'js/src/vm/UnboxedObject.cpp',
      'js/src/vm/Unicode.cpp',
      'js/src/vm/Value.cpp',
      'js/src/vm/WeakMapPtr.cpp',
      'js/src/vm/Xdr.cpp',
      'mfbt/decimal/Decimal.cpp',
      'mfbt/double-conversion/bignum-dtoa.cc',
      'mfbt/double-conversion/bignum.cc',
      'mfbt/double-conversion/cached-powers.cc',
      'mfbt/double-conversion/diy-fp.cc',
      'mfbt/double-conversion/double-conversion.cc',
      'mfbt/double-conversion/fast-dtoa.cc',
      'mfbt/double-conversion/fixed-dtoa.cc',
      'mfbt/double-conversion/strtod.cc',
      'mfbt/FloatingPoint.cpp',
      'mfbt/HashFunctions.cpp',
      # Note that this file was renamed from 'mfbt/Compression.cpp'.  Some
      # build systems, e.g. MSVC08, cannot handle several files with the
      # same basename.
      'mfbt/mfbt_Compression.cpp',
      'mfbt/unused.cpp',
      'mozglue/misc/TimeStamp.cpp',
      'mozglue/misc/TimeStamp_starboard.cpp',
    ],
  },
}
