// Copyright 2021 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "Halide.h"
#include <stdio.h>

int main(int argc, char **argv) {
  // Input arguments
  Halide::Param<uint32_t> center_x;
  Halide::Param<uint32_t> center_y;
  std::vector<Halide::Argument> args = {center_x, center_y};

  // Pipeline definitions
  Halide::Var x, y;
  Halide::Func gradient;

  // Calculate a spherical gradient amount the center coordinates
  Halide::Expr offset =
      Halide::pow(x - center_x, 2) + Halide::pow(y - center_y, 2);

  // Cast the result to uint_8
  gradient(x, y) = Halide::cast<uint8_t>(offset);

  // Quickly test the pipeline
  Halide::ParamMap params;
  params.set(center_x, (uint32_t)7);
  params.set(center_y, (uint32_t)7);
  Halide::Buffer<uint8_t> output = gradient.realize(
      {15, 21}, Halide::get_jit_target_from_environment(), params);

  for (int j = 0; j < output.height(); j++) {
    for (int i = 0; i < output.width(); i++) {
      printf("%2x ", output(i, j));
    }
    printf("\n");
  }

  // Cross compile for RISC-V
  Halide::Target target;
  target.os = Halide::Target::NoOS;
  target.arch = Halide::Target::RISCV;
  target.bits = 32;
  std::vector<Halide::Target::Feature> riscv_features;

  // En/Disable some features
  // riscv_features.push_back(Halide::Target::Feature::Debug);
  riscv_features.push_back(Halide::Target::Feature::NoAsserts);
  riscv_features.push_back(Halide::Target::Feature::NoBoundsQuery);
  riscv_features.push_back(Halide::Target::Feature::NoRuntime);
  riscv_features.push_back(Halide::Target::Feature::RISCV_A);
  riscv_features.push_back(Halide::Target::Feature::RISCV_M);
  riscv_features.push_back(Halide::Target::Feature::SoftFloatABI);
  target.set_features(riscv_features);

  // Compile
  gradient.compile_to_file("halide_pipeline.riscv", args, "gradient", target);
  gradient.compile_to_llvm_assembly("halide_pipeline.riscv.ll", args,
                                    "halide_pipeline", target);
  gradient.compile_to_assembly("halide_pipeline.riscv.s", args,
                               "halide_pipeline", target);
  gradient.compile_to_c("halide_pipeline.riscv.c", args, "gradient", target);
  gradient.compile_to_lowered_stmt("halide_pipeline.riscv.html", args,
                                   Halide::HTML, target);
  printf("Cross-compilation successful!\n");

  return 0;
}
