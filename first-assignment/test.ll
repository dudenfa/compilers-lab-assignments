; ModuleID = 'test.c'
source_filename = "test.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx15.0.0"

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i32, align 4
  store i32 15, ptr %1, align 4
  %10 = load i32, ptr %1, align 4
  %11 = add nsw i32 %10, 0
  store i32 %11, ptr %2, align 4
  %12 = load i32, ptr %1, align 4
  %13 = add nsw i32 0, %12
  store i32 %13, ptr %3, align 4
  %14 = load i32, ptr %1, align 4
  %15 = sub nsw i32 %14, 0
  store i32 %15, ptr %4, align 4
  %16 = load i32, ptr %1, align 4
  %17 = sub nsw i32 0, %16
  store i32 %17, ptr %5, align 4
  %18 = load i32, ptr %1, align 4
  %19 = mul nsw i32 %18, 1
  store i32 %19, ptr %6, align 4
  %20 = load i32, ptr %1, align 4
  %21 = mul nsw i32 1, %20
  store i32 %21, ptr %7, align 4
  %22 = load i32, ptr %1, align 4
  %23 = sdiv i32 %22, 1
  store i32 %23, ptr %8, align 4
  %24 = load i32, ptr %1, align 4
  %25 = sdiv i32 1, %24
  store i32 %25, ptr %9, align 4
  ret i32 0
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
