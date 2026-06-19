; ModuleID = 'tests/test.ll'
source_filename = "tests/test.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx15.0.0"

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_basic_hoist(i32 noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  %4 = mul nsw i32 %0, %1
  br label %5

5:                                                ; preds = %10, %3
  %.01 = phi i32 [ 0, %3 ], [ %9, %10 ]
  %.0 = phi i32 [ 0, %3 ], [ %11, %10 ]
  %6 = icmp slt i32 %.0, %2
  br i1 %6, label %7, label %12

7:                                                ; preds = %5
  %8 = add nsw i32 %4, %.0
  %9 = add nsw i32 %.01, %8
  br label %10

10:                                               ; preds = %7
  %11 = add nsw i32 %.0, 1
  br label %5, !llvm.loop !6

12:                                               ; preds = %5
  ret i32 %.01
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_invariant_chain(i32 noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  %4 = add nsw i32 %0, %1
  %5 = mul nsw i32 %4, 42
  br label %6

6:                                                ; preds = %11, %3
  %.01 = phi i32 [ 0, %3 ], [ %10, %11 ]
  %.0 = phi i32 [ 0, %3 ], [ %12, %11 ]
  %7 = icmp slt i32 %.0, %2
  br i1 %7, label %8, label %13

8:                                                ; preds = %6
  %9 = add nsw i32 %5, %.0
  %10 = add nsw i32 %.01, %9
  br label %11

11:                                               ; preds = %8
  %12 = add nsw i32 %.0, 1
  br label %6, !llvm.loop !8

13:                                               ; preds = %6
  ret i32 %.01
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_safe_vs_unsafe(i32 noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  %4 = mul nsw i32 %0, %1
  br label %5

5:                                                ; preds = %12, %3
  %.01 = phi i32 [ 0, %3 ], [ %11, %12 ]
  %.0 = phi i32 [ 0, %3 ], [ %13, %12 ]
  %6 = icmp slt i32 %.0, %2
  br i1 %6, label %7, label %14

7:                                                ; preds = %5
  %8 = sdiv i32 %0, %1
  %9 = add nsw i32 %4, %8
  %10 = add nsw i32 %9, %.0
  %11 = add nsw i32 %.01, %10
  br label %12

12:                                               ; preds = %7
  %13 = add nsw i32 %.0, 1
  br label %5, !llvm.loop !9

14:                                               ; preds = %5
  ret i32 %.01
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_dominance_needed(i32 noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  %4 = sdiv i32 %0, %1
  br label %5

5:                                                ; preds = %9, %3
  %.01 = phi i32 [ 0, %3 ], [ %7, %9 ]
  %.0 = phi i32 [ 0, %3 ], [ %8, %9 ]
  %6 = add nsw i32 %4, %.0
  %7 = add nsw i32 %.01, %6
  %8 = add nsw i32 %.0, 1
  br label %9

9:                                                ; preds = %5
  %10 = icmp slt i32 %8, %2
  br i1 %10, label %5, label %11, !llvm.loop !10

11:                                               ; preds = %9
  ret i32 %7
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @test_memory_no_hoist(ptr noundef %0, i32 noundef %1) #0 {
  br label %3

3:                                                ; preds = %7, %2
  %.0 = phi i32 [ 0, %2 ], [ %8, %7 ]
  %4 = icmp slt i32 %.0, %1
  br i1 %4, label %5, label %9

5:                                                ; preds = %3
  %6 = load i32, ptr %0, align 4
  store i32 42, ptr %0, align 4
  br label %7

7:                                                ; preds = %5
  %8 = add nsw i32 %.0, 1
  br label %3, !llvm.loop !11

9:                                                ; preds = %3
  ret void
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @test_nested_hoist(i32 noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  %4 = add nsw i32 %0, %1
  br label %5

5:                                                ; preds = %16, %3
  %.02 = phi i32 [ 0, %3 ], [ %.1, %16 ]
  %.01 = phi i32 [ 0, %3 ], [ %17, %16 ]
  %6 = icmp slt i32 %.01, %2
  br i1 %6, label %7, label %18

7:                                                ; preds = %5
  br label %8

8:                                                ; preds = %13, %7
  %.1 = phi i32 [ %.02, %7 ], [ %12, %13 ]
  %.0 = phi i32 [ 0, %7 ], [ %14, %13 ]
  %9 = icmp slt i32 %.0, %2
  br i1 %9, label %10, label %15

10:                                               ; preds = %8
  %11 = add nsw i32 %4, %.0
  %12 = add nsw i32 %.1, %11
  br label %13

13:                                               ; preds = %10
  %14 = add nsw i32 %.0, 1
  br label %8, !llvm.loop !12

15:                                               ; preds = %8
  br label %16

16:                                               ; preds = %15
  %17 = add nsw i32 %.01, 1
  br label %5, !llvm.loop !13

18:                                               ; preds = %5
  ret i32 %.02
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
