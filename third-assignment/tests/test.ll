; ModuleID = 'tests/test.c'
source_filename = "tests/test.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx15.0.0"

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_basic_hoist(i32 noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i32, align 4
  store i32 %0, ptr %4, align 4
  store i32 %1, ptr %5, align 4
  store i32 %2, ptr %6, align 4
  store i32 0, ptr %7, align 4
  store i32 0, ptr %8, align 4
  br label %10

10:                                               ; preds = %23, %3
  %11 = load i32, ptr %8, align 4
  %12 = load i32, ptr %6, align 4
  %13 = icmp slt i32 %11, %12
  br i1 %13, label %14, label %26

14:                                               ; preds = %10
  %15 = load i32, ptr %4, align 4
  %16 = load i32, ptr %5, align 4
  %17 = mul nsw i32 %15, %16
  store i32 %17, ptr %9, align 4
  %18 = load i32, ptr %9, align 4
  %19 = load i32, ptr %8, align 4
  %20 = add nsw i32 %18, %19
  %21 = load i32, ptr %7, align 4
  %22 = add nsw i32 %21, %20
  store i32 %22, ptr %7, align 4
  br label %23

23:                                               ; preds = %14
  %24 = load i32, ptr %8, align 4
  %25 = add nsw i32 %24, 1
  store i32 %25, ptr %8, align 4
  br label %10, !llvm.loop !6

26:                                               ; preds = %10
  %27 = load i32, ptr %7, align 4
  ret i32 %27
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_invariant_chain(i32 noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i32, align 4
  %10 = alloca i32, align 4
  store i32 %0, ptr %4, align 4
  store i32 %1, ptr %5, align 4
  store i32 %2, ptr %6, align 4
  store i32 0, ptr %7, align 4
  store i32 0, ptr %8, align 4
  br label %11

11:                                               ; preds = %26, %3
  %12 = load i32, ptr %8, align 4
  %13 = load i32, ptr %6, align 4
  %14 = icmp slt i32 %12, %13
  br i1 %14, label %15, label %29

15:                                               ; preds = %11
  %16 = load i32, ptr %4, align 4
  %17 = load i32, ptr %5, align 4
  %18 = add nsw i32 %16, %17
  store i32 %18, ptr %9, align 4
  %19 = load i32, ptr %9, align 4
  %20 = mul nsw i32 %19, 42
  store i32 %20, ptr %10, align 4
  %21 = load i32, ptr %10, align 4
  %22 = load i32, ptr %8, align 4
  %23 = add nsw i32 %21, %22
  %24 = load i32, ptr %7, align 4
  %25 = add nsw i32 %24, %23
  store i32 %25, ptr %7, align 4
  br label %26

26:                                               ; preds = %15
  %27 = load i32, ptr %8, align 4
  %28 = add nsw i32 %27, 1
  store i32 %28, ptr %8, align 4
  br label %11, !llvm.loop !8

29:                                               ; preds = %11
  %30 = load i32, ptr %7, align 4
  ret i32 %30
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_safe_vs_unsafe(i32 noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i32, align 4
  %10 = alloca i32, align 4
  store i32 %0, ptr %4, align 4
  store i32 %1, ptr %5, align 4
  store i32 %2, ptr %6, align 4
  store i32 0, ptr %7, align 4
  store i32 0, ptr %8, align 4
  br label %11

11:                                               ; preds = %29, %3
  %12 = load i32, ptr %8, align 4
  %13 = load i32, ptr %6, align 4
  %14 = icmp slt i32 %12, %13
  br i1 %14, label %15, label %32

15:                                               ; preds = %11
  %16 = load i32, ptr %4, align 4
  %17 = load i32, ptr %5, align 4
  %18 = mul nsw i32 %16, %17
  store i32 %18, ptr %9, align 4
  %19 = load i32, ptr %4, align 4
  %20 = load i32, ptr %5, align 4
  %21 = sdiv i32 %19, %20
  store i32 %21, ptr %10, align 4
  %22 = load i32, ptr %9, align 4
  %23 = load i32, ptr %10, align 4
  %24 = add nsw i32 %22, %23
  %25 = load i32, ptr %8, align 4
  %26 = add nsw i32 %24, %25
  %27 = load i32, ptr %7, align 4
  %28 = add nsw i32 %27, %26
  store i32 %28, ptr %7, align 4
  br label %29

29:                                               ; preds = %15
  %30 = load i32, ptr %8, align 4
  %31 = add nsw i32 %30, 1
  store i32 %31, ptr %8, align 4
  br label %11, !llvm.loop !9

32:                                               ; preds = %11
  %33 = load i32, ptr %7, align 4
  ret i32 %33
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_dominance_needed(i32 noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i32, align 4
  store i32 %0, ptr %4, align 4
  store i32 %1, ptr %5, align 4
  store i32 %2, ptr %6, align 4
  store i32 0, ptr %7, align 4
  store i32 0, ptr %8, align 4
  br label %10

10:                                               ; preds = %21, %3
  %11 = load i32, ptr %4, align 4
  %12 = load i32, ptr %5, align 4
  %13 = sdiv i32 %11, %12
  store i32 %13, ptr %9, align 4
  %14 = load i32, ptr %9, align 4
  %15 = load i32, ptr %8, align 4
  %16 = add nsw i32 %14, %15
  %17 = load i32, ptr %7, align 4
  %18 = add nsw i32 %17, %16
  store i32 %18, ptr %7, align 4
  %19 = load i32, ptr %8, align 4
  %20 = add nsw i32 %19, 1
  store i32 %20, ptr %8, align 4
  br label %21

21:                                               ; preds = %10
  %22 = load i32, ptr %8, align 4
  %23 = load i32, ptr %6, align 4
  %24 = icmp slt i32 %22, %23
  br i1 %24, label %10, label %25, !llvm.loop !10

25:                                               ; preds = %21
  %26 = load i32, ptr %7, align 4
  ret i32 %26
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @test_memory_no_hoist(ptr noundef %0, i32 noundef %1) #0 {
  %3 = alloca ptr, align 8
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  store ptr %0, ptr %3, align 8
  store i32 %1, ptr %4, align 4
  store i32 0, ptr %5, align 4
  br label %7

7:                                                ; preds = %16, %2
  %8 = load i32, ptr %5, align 4
  %9 = load i32, ptr %4, align 4
  %10 = icmp slt i32 %8, %9
  br i1 %10, label %11, label %19

11:                                               ; preds = %7
  %12 = load ptr, ptr %3, align 8
  %13 = load i32, ptr %12, align 4
  store i32 %13, ptr %6, align 4
  %14 = load ptr, ptr %3, align 8
  store i32 42, ptr %14, align 4
  %15 = load i32, ptr %6, align 4
  br label %16

16:                                               ; preds = %11
  %17 = load i32, ptr %5, align 4
  %18 = add nsw i32 %17, 1
  store i32 %18, ptr %5, align 4
  br label %7, !llvm.loop !11

19:                                               ; preds = %7
  ret void
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_nested_hoist(i32 noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i32, align 4
  %10 = alloca i32, align 4
  store i32 %0, ptr %4, align 4
  store i32 %1, ptr %5, align 4
  store i32 %2, ptr %6, align 4
  store i32 0, ptr %7, align 4
  store i32 0, ptr %8, align 4
  br label %11

11:                                               ; preds = %33, %3
  %12 = load i32, ptr %8, align 4
  %13 = load i32, ptr %6, align 4
  %14 = icmp slt i32 %12, %13
  br i1 %14, label %15, label %36

15:                                               ; preds = %11
  store i32 0, ptr %9, align 4
  br label %16

16:                                               ; preds = %29, %15
  %17 = load i32, ptr %9, align 4
  %18 = load i32, ptr %6, align 4
  %19 = icmp slt i32 %17, %18
  br i1 %19, label %20, label %32

20:                                               ; preds = %16
  %21 = load i32, ptr %4, align 4
  %22 = load i32, ptr %5, align 4
  %23 = add nsw i32 %21, %22
  store i32 %23, ptr %10, align 4
  %24 = load i32, ptr %10, align 4
  %25 = load i32, ptr %9, align 4
  %26 = add nsw i32 %24, %25
  %27 = load i32, ptr %7, align 4
  %28 = add nsw i32 %27, %26
  store i32 %28, ptr %7, align 4
  br label %29

29:                                               ; preds = %20
  %30 = load i32, ptr %9, align 4
  %31 = add nsw i32 %30, 1
  store i32 %31, ptr %9, align 4
  br label %16, !llvm.loop !12

32:                                               ; preds = %16
  br label %33

33:                                               ; preds = %32
  %34 = load i32, ptr %8, align 4
  %35 = add nsw i32 %34, 1
  store i32 %35, ptr %8, align 4
  br label %11, !llvm.loop !13

36:                                               ; preds = %11
  %37 = load i32, ptr %7, align 4
  ret i32 %37
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
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
!9 = distinct !{!9, !7}
!10 = distinct !{!10, !7}
!11 = distinct !{!11, !7}
!12 = distinct !{!12, !7}
!13 = distinct !{!13, !7}
