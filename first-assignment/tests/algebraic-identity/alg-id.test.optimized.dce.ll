; ModuleID = '/Users/mmantovani/Desktop/uni/Compilatori/Laboratori-Compilatori/compilers-lab-assignments/first-assignment/tests/algebraic-identity/test.ll'
source_filename = "/Users/mmantovani/Desktop/uni/Compilatori/Laboratori-Compilatori/compilers-lab-assignments/first-assignment/tests/algebraic-identity/test.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx15.0.0"

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @add(i32 noundef %0, i32 noundef %1) #0 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  store i32 %1, ptr %4, align 4
  %8 = load i32, ptr %3, align 4
  store i32 %8, ptr %5, align 4
  %9 = load i32, ptr %3, align 4
  store i32 %9, ptr %6, align 4
  %10 = load i32, ptr %3, align 4
  %11 = load i32, ptr %4, align 4
  %12 = add nsw i32 %10, %11
  store i32 %12, ptr %7, align 4
  %13 = load i32, ptr %5, align 4
  %14 = load i32, ptr %6, align 4
  %15 = add nsw i32 %13, %14
  %16 = load i32, ptr %7, align 4
  %17 = add nsw i32 %15, %16
  ret i32 %17
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @sub(i32 noundef %0, i32 noundef %1) #0 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  store i32 %1, ptr %4, align 4
  %8 = load i32, ptr %3, align 4
  store i32 %8, ptr %5, align 4
  %9 = load i32, ptr %3, align 4
  %10 = sub nsw i32 0, %9
  store i32 %10, ptr %6, align 4
  %11 = load i32, ptr %3, align 4
  %12 = load i32, ptr %4, align 4
  %13 = sub nsw i32 %11, %12
  store i32 %13, ptr %7, align 4
  %14 = load i32, ptr %5, align 4
  %15 = load i32, ptr %6, align 4
  %16 = add nsw i32 %14, %15
  %17 = load i32, ptr %7, align 4
  %18 = add nsw i32 %16, %17
  ret i32 %18
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @mul(i32 noundef %0, i32 noundef %1) #0 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  store i32 %1, ptr %4, align 4
  %9 = load i32, ptr %3, align 4
  store i32 %9, ptr %5, align 4
  %10 = load i32, ptr %3, align 4
  store i32 %10, ptr %6, align 4
  %11 = load i32, ptr %3, align 4
  %12 = mul nsw i32 %11, 0
  store i32 %12, ptr %7, align 4
  %13 = load i32, ptr %3, align 4
  %14 = load i32, ptr %4, align 4
  %15 = mul nsw i32 %13, %14
  store i32 %15, ptr %8, align 4
  %16 = load i32, ptr %5, align 4
  %17 = load i32, ptr %6, align 4
  %18 = add nsw i32 %16, %17
  %19 = load i32, ptr %7, align 4
  %20 = add nsw i32 %18, %19
  %21 = load i32, ptr %8, align 4
  %22 = add nsw i32 %20, %21
  ret i32 %22
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @div_test(i32 noundef %0, i32 noundef %1) #0 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  store i32 %1, ptr %4, align 4
  %9 = load i32, ptr %3, align 4
  store i32 %9, ptr %5, align 4
  %10 = load i32, ptr %3, align 4
  %11 = sdiv i32 0, %10
  store i32 %11, ptr %6, align 4
  %12 = load i32, ptr %3, align 4
  %13 = sdiv i32 1, %12
  store i32 %13, ptr %7, align 4
  %14 = load i32, ptr %3, align 4
  %15 = load i32, ptr %4, align 4
  %16 = sdiv i32 %14, %15
  store i32 %16, ptr %8, align 4
  %17 = load i32, ptr %5, align 4
  %18 = load i32, ptr %6, align 4
  %19 = add nsw i32 %17, %18
  %20 = load i32, ptr %7, align 4
  %21 = add nsw i32 %19, %20
  %22 = load i32, ptr %8, align 4
  %23 = add nsw i32 %21, %22
  ret i32 %23
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @nested(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %6 = load i32, ptr %2, align 4
  store i32 %6, ptr %3, align 4
  %7 = load i32, ptr %3, align 4
  store i32 %7, ptr %4, align 4
  %8 = load i32, ptr %4, align 4
  store i32 %8, ptr %5, align 4
  %9 = load i32, ptr %5, align 4
  ret i32 %9
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
