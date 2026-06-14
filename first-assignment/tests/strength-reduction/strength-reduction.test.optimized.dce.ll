; ModuleID = '/Users/mmantovani/Desktop/uni/Compilatori/Laboratori-Compilatori/compilers-lab-assignments/first-assignment/tests/strength-reduction/test.ll'
source_filename = "/Users/mmantovani/Desktop/uni/Compilatori/Laboratori-Compilatori/compilers-lab-assignments/first-assignment/tests/strength-reduction/test.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx15.0.0"

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_mul(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %8 = load i32, ptr %2, align 4
  %sr.shl = shl i32 %8, 3
  store i32 %sr.shl, ptr %3, align 4
  %9 = load i32, ptr %2, align 4
  %sr.shl1 = shl i32 %9, 3
  %sr.add = add i32 %sr.shl1, %9
  store i32 %sr.add, ptr %4, align 4
  %10 = load i32, ptr %2, align 4
  %sr.shl2 = shl i32 %10, 3
  %sr.sub = sub i32 %sr.shl2, %10
  store i32 %sr.sub, ptr %5, align 4
  %11 = load i32, ptr %2, align 4
  %sr.shl3 = shl i32 %11, 4
  %sr.sub4 = sub i32 %sr.shl3, %11
  store i32 %sr.sub4, ptr %6, align 4
  %12 = load i32, ptr %2, align 4
  %13 = mul nsw i32 %12, 6
  store i32 %13, ptr %7, align 4
  %14 = load i32, ptr %3, align 4
  %15 = load i32, ptr %4, align 4
  %16 = add nsw i32 %14, %15
  %17 = load i32, ptr %5, align 4
  %18 = add nsw i32 %16, %17
  %19 = load i32, ptr %6, align 4
  %20 = add nsw i32 %18, %19
  %21 = load i32, ptr %7, align 4
  %22 = add nsw i32 %20, %21
  ret i32 %22
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_udiv(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %5 = load i32, ptr %2, align 4
  %sr.lshr = lshr i32 %5, 2
  store i32 %sr.lshr, ptr %3, align 4
  %6 = load i32, ptr %2, align 4
  %7 = udiv i32 %6, 6
  store i32 %7, ptr %4, align 4
  %8 = load i32, ptr %3, align 4
  %9 = load i32, ptr %4, align 4
  %10 = add i32 %8, %9
  ret i32 %10
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_sdiv(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %5 = load i32, ptr %2, align 4
  %sr.ashr = ashr i32 %5, 3
  store i32 %sr.ashr, ptr %3, align 4
  %6 = load i32, ptr %2, align 4
  %7 = sdiv i32 %6, 3
  store i32 %7, ptr %4, align 4
  %8 = load i32, ptr %3, align 4
  %9 = load i32, ptr %4, align 4
  %10 = add nsw i32 %8, %9
  ret i32 %10
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_no_opt(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %5 = load i32, ptr %2, align 4
  %6 = mul nsw i32 %5, -8
  store i32 %6, ptr %3, align 4
  %7 = load i32, ptr %2, align 4
  %8 = sdiv i32 %7, -4
  store i32 %8, ptr %4, align 4
  %9 = load i32, ptr %3, align 4
  %10 = load i32, ptr %4, align 4
  %11 = add nsw i32 %9, %10
  ret i32 %11
}

attributes #0 = { noinline nounwind ssp uwtable(sync) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+altnzcv,+ccdp,+ccidx,+complxnum,+crc,+dit,+dotprod,+flagm,+fp-armv8,+fp16fml,+fptoint,+fullfp16,+jsconv,+lse,+neon,+pauth,+perfmon,+predres,+ras,+rcpc,+rdm,+sb,+sha2,+sha3,+specrestrict,+ssbs,+v8.1a,+v8.2a,+v8.3a,+v8.4a,+v8a,+zcm,+zcz" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 2, !"SDK Version", [2 x i32] [i32 15, i32 5]}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 8, !"PIC Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 1}
!5 = !{!"Homebrew clang version 19.1.7"}
