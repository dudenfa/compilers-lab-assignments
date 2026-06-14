; ModuleID = '/Users/mmantovani/Desktop/uni/Compilatori/Laboratori-Compilatori/compilers-lab-assignments/first-assignment/tests/multi-inst-opt/test.c'
source_filename = "/Users/mmantovani/Desktop/uni/Compilatori/Laboratori-Compilatori/compilers-lab-assignments/first-assignment/tests/multi-inst-opt/test.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx15.0.0"

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_assignment(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %5 = load i32, ptr %2, align 4
  %6 = add nsw i32 %5, 1
  store i32 %6, ptr %3, align 4
  %7 = load i32, ptr %3, align 4
  %8 = sub nsw i32 %7, 1
  store i32 %8, ptr %4, align 4
  %9 = load i32, ptr %4, align 4
  ret i32 %9
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_add_sub(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %4 = load i32, ptr %2, align 4
  %5 = add nsw i32 %4, 5
  store i32 %5, ptr %3, align 4
  %6 = load i32, ptr %3, align 4
  %7 = sub nsw i32 %6, 5
  ret i32 %7
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_add_sub_comm(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %4 = load i32, ptr %2, align 4
  %5 = add nsw i32 5, %4
  store i32 %5, ptr %3, align 4
  %6 = load i32, ptr %3, align 4
  %7 = sub nsw i32 %6, 5
  ret i32 %7
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_sub_add(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %4 = load i32, ptr %2, align 4
  %5 = sub nsw i32 %4, 10
  store i32 %5, ptr %3, align 4
  %6 = load i32, ptr %3, align 4
  %7 = add nsw i32 %6, 10
  ret i32 %7
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_mul_sdiv(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %4 = load i32, ptr %2, align 4
  %5 = mul nsw i32 %4, 4
  store i32 %5, ptr %3, align 4
  %6 = load i32, ptr %3, align 4
  %7 = sdiv i32 %6, 4
  ret i32 %7
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_mul_sdiv_comm(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %4 = load i32, ptr %2, align 4
  %5 = mul nsw i32 4, %4
  store i32 %5, ptr %3, align 4
  %6 = load i32, ptr %3, align 4
  %7 = sdiv i32 %6, 4
  ret i32 %7
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_mul_udiv(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %4 = load i32, ptr %2, align 4
  %5 = mul i32 %4, 8
  store i32 %5, ptr %3, align 4
  %6 = load i32, ptr %3, align 4
  %7 = udiv i32 %6, 8
  ret i32 %7
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_wrong_constants(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %4 = load i32, ptr %2, align 4
  %5 = add nsw i32 %4, 5
  store i32 %5, ptr %3, align 4
  %6 = load i32, ptr %3, align 4
  %7 = sub nsw i32 %6, 4
  ret i32 %7
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_sub_wrong_order(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %4 = load i32, ptr %2, align 4
  %5 = sub nsw i32 10, %4
  store i32 %5, ptr %3, align 4
  %6 = load i32, ptr %3, align 4
  %7 = add nsw i32 %6, 10
  ret i32 %7
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_div_mul_no_opt(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %4 = load i32, ptr %2, align 4
  %5 = sdiv i32 %4, 3
  store i32 %5, ptr %3, align 4
  %6 = load i32, ptr %3, align 4
  %7 = mul nsw i32 %6, 3
  ret i32 %7
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_div_mul_power2_no_opt(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %4 = load i32, ptr %2, align 4
  %5 = sdiv i32 %4, 8
  store i32 %5, ptr %3, align 4
  %6 = load i32, ptr %3, align 4
  %7 = mul nsw i32 %6, 8
  ret i32 %7
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
