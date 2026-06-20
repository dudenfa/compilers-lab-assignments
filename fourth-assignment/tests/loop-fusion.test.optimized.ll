; ModuleID = 'tests/test.ll'
source_filename = "tests/test.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx15.0.0"

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @test_simple_fuse(ptr noalias noundef %0, ptr noalias noundef %1, i64 noundef %2) #0 {
  br label %4

4:                                                ; preds = %6, %3
  %.01 = phi i64 [ 0, %3 ], [ %12, %6 ]
  %5 = icmp ult i64 %.01, %2
  br i1 %5, label %6, label %13

6:                                                ; preds = %4
  %7 = trunc i64 %.01 to i32
  %8 = getelementptr inbounds i32, ptr %0, i64 %.01
  store i32 %7, ptr %8, align 4
  %9 = getelementptr inbounds i32, ptr %0, i64 %.01
  %10 = load i32, ptr %9, align 4
  %11 = getelementptr inbounds i32, ptr %1, i64 %.01
  store i32 %10, ptr %11, align 4
  %12 = add i64 %.01, 1
  br label %4, !llvm.loop !6

13:                                               ; preds = %4
  ret void
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @test_positive_dependence(ptr noalias noundef %0, i32 noundef %1, i64 noundef %2) #0 {
  br label %4

4:                                                ; preds = %6, %3
  %.01 = phi i64 [ 0, %3 ], [ %12, %6 ]
  %5 = icmp ult i64 %.01, %2
  br i1 %5, label %6, label %13

6:                                                ; preds = %4
  %7 = trunc i64 %.01 to i32
  %8 = getelementptr inbounds i32, ptr %0, i64 %.01
  store i32 %7, ptr %8, align 4
  %9 = sub i64 %.01, 1
  %10 = getelementptr inbounds i32, ptr %0, i64 %9
  %11 = load i32, ptr %10, align 4
  %12 = add i64 %.01, 1
  br label %4, !llvm.loop !8

13:                                               ; preds = %4
  ret void
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @test_negative_dependence(ptr noalias noundef %0, i32 noundef %1, i64 noundef %2) #0 {
  br label %4

4:                                                ; preds = %6, %3
  %.01 = phi i64 [ 0, %3 ], [ %9, %6 ]
  %5 = icmp ult i64 %.01, %2
  br i1 %5, label %6, label %10

6:                                                ; preds = %4
  %7 = trunc i64 %.01 to i32
  %8 = getelementptr inbounds i32, ptr %0, i64 %.01
  store i32 %7, ptr %8, align 4
  %9 = add i64 %.01, 1
  br label %4, !llvm.loop !9

10:                                               ; preds = %4, %12
  %.0 = phi i64 [ %16, %12 ], [ 0, %4 ]
  %11 = icmp ult i64 %.0, %2
  br i1 %11, label %12, label %17

12:                                               ; preds = %10
  %13 = add i64 %.0, 1
  %14 = getelementptr inbounds i32, ptr %0, i64 %13
  %15 = load i32, ptr %14, align 4
  %16 = add i64 %.0, 1
  br label %10, !llvm.loop !10

17:                                               ; preds = %10
  ret void
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @test_different_trip_count(i32 noundef %0, i32 noundef %1, i64 noundef %2, i64 noundef %3) #0 {
  br label %5

5:                                                ; preds = %7, %4
  %.01 = phi i64 [ 0, %4 ], [ %8, %7 ]
  %6 = icmp ult i64 %.01, %2
  br i1 %6, label %7, label %9

7:                                                ; preds = %5
  %8 = add i64 %.01, 1
  br label %5, !llvm.loop !11

9:                                                ; preds = %5, %11
  %.0 = phi i64 [ %12, %11 ], [ 0, %5 ]
  %10 = icmp ult i64 %.0, %3
  br i1 %10, label %11, label %13

11:                                               ; preds = %9
  %12 = add i64 %.0, 1
  br label %9, !llvm.loop !12

13:                                               ; preds = %9
  ret void
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @test_not_adjacent(i32 noundef %0, i32 noundef %1, i32 noundef %2, i64 noundef %3) #0 {
  br label %5

5:                                                ; preds = %7, %4
  %.02 = phi i64 [ 0, %4 ], [ %8, %7 ]
  %.0 = phi i32 [ %0, %4 ], [ 1, %7 ]
  %6 = icmp ult i64 %.02, %3
  br i1 %6, label %7, label %9

7:                                                ; preds = %5
  %8 = add i64 %.02, 1
  br label %5, !llvm.loop !13

9:                                                ; preds = %5
  %10 = add nsw i32 %.0, %1
  br label %11

11:                                               ; preds = %13, %9
  %.01 = phi i64 [ 0, %9 ], [ %14, %13 ]
  %12 = icmp ult i64 %.01, %3
  br i1 %12, label %13, label %15

13:                                               ; preds = %11
  %14 = add i64 %.01, 1
  br label %11, !llvm.loop !14

15:                                               ; preds = %11
  ret void
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @test_write_after_write(ptr noalias noundef %0, i32 noundef %1, i64 noundef %2) #0 {
  br label %4

4:                                                ; preds = %6, %3
  %.01 = phi i64 [ 0, %3 ], [ %8, %6 ]
  %5 = icmp ult i64 %.01, %2
  br i1 %5, label %6, label %9

6:                                                ; preds = %4
  %7 = getelementptr inbounds i32, ptr %0, i64 %.01
  store i32 1, ptr %7, align 4
  %8 = add i64 %.01, 1
  br label %4, !llvm.loop !15

9:                                                ; preds = %4, %11
  %.0 = phi i64 [ %14, %11 ], [ 0, %4 ]
  %10 = icmp ult i64 %.0, %2
  br i1 %10, label %11, label %15

11:                                               ; preds = %9
  %12 = add i64 %.0, 1
  %13 = getelementptr inbounds i32, ptr %0, i64 %12
  store i32 %1, ptr %13, align 4
  %14 = add i64 %.0, 1
  br label %9, !llvm.loop !16

15:                                               ; preds = %9
  ret void
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @test_write_after_read(ptr noalias noundef %0, i32 noundef %1, i32 noundef %2, i64 noundef %3) #0 {
  br label %5

5:                                                ; preds = %7, %4
  %.01 = phi i64 [ 0, %4 ], [ %10, %7 ]
  %6 = icmp ult i64 %.01, %3
  br i1 %6, label %7, label %11

7:                                                ; preds = %5
  %8 = getelementptr inbounds i32, ptr %0, i64 %.01
  %9 = load i32, ptr %8, align 4
  %10 = add i64 %.01, 1
  br label %5, !llvm.loop !17

11:                                               ; preds = %5, %13
  %.0 = phi i64 [ %16, %13 ], [ 0, %5 ]
  %12 = icmp ult i64 %.0, %3
  br i1 %12, label %13, label %17

13:                                               ; preds = %11
  %14 = add i64 %.0, 1
  %15 = getelementptr inbounds i32, ptr %0, i64 %14
  store i32 %2, ptr %15, align 4
  %16 = add i64 %.0, 1
  br label %11, !llvm.loop !18

17:                                               ; preds = %11
  ret void
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @test_constant_access(ptr noalias noundef %0, i32 noundef %1, i64 noundef %2) #0 {
  br label %4

4:                                                ; preds = %6, %3
  %.01 = phi i64 [ 0, %3 ], [ %10, %6 ]
  %5 = icmp ult i64 %.01, %2
  br i1 %5, label %6, label %11

6:                                                ; preds = %4
  %7 = getelementptr inbounds i32, ptr %0, i64 0
  store i32 1, ptr %7, align 4
  %8 = getelementptr inbounds i32, ptr %0, i64 1
  %9 = load i32, ptr %8, align 4
  %10 = add i64 %.01, 1
  br label %4, !llvm.loop !19

11:                                               ; preds = %4
  ret void
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @test_mixed_access(ptr noalias noundef %0, i32 noundef %1, i64 noundef %2) #0 {
  br label %4

4:                                                ; preds = %6, %3
  %.01 = phi i64 [ 0, %3 ], [ %8, %6 ]
  %5 = icmp ult i64 %.01, %2
  br i1 %5, label %6, label %9

6:                                                ; preds = %4
  %7 = getelementptr inbounds i32, ptr %0, i64 0
  store i32 1, ptr %7, align 4
  %8 = add i64 %.01, 1
  br label %4, !llvm.loop !20

9:                                                ; preds = %4, %11
  %.0 = phi i64 [ %14, %11 ], [ 0, %4 ]
  %10 = icmp ult i64 %.0, %2
  br i1 %10, label %11, label %15

11:                                               ; preds = %9
  %12 = getelementptr inbounds i32, ptr %0, i64 %.0
  %13 = load i32, ptr %12, align 4
  %14 = add i64 %.0, 1
  br label %9, !llvm.loop !21

15:                                               ; preds = %9
  ret void
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @test_while_fuse(ptr noalias noundef %0, ptr noalias noundef %1, i64 noundef %2) #0 {
  br label %4

4:                                                ; preds = %6, %3
  %.0 = phi i64 [ 0, %3 ], [ %9, %6 ]
  %5 = icmp ult i64 %.0, %2
  br i1 %5, label %6, label %10

6:                                                ; preds = %4
  %7 = trunc i64 %.0 to i32
  %8 = getelementptr inbounds i32, ptr %0, i64 %.0
  store i32 %7, ptr %8, align 4
  %9 = add i64 %.0, 1
  br label %4, !llvm.loop !22

10:                                               ; preds = %4
  ret void
}

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @test_dowhile_no_fuse(ptr noalias noundef %0, i32 noundef %1, i64 noundef %2) #0 {
  br label %4

4:                                                ; preds = %4, %3
  %.0 = phi i64 [ 0, %3 ], [ %7, %4 ]
  %5 = trunc i64 %.0 to i32
  %6 = getelementptr inbounds i32, ptr %0, i64 %.0
  store i32 %5, ptr %6, align 4
  %7 = add i64 %.0, 1
  %8 = icmp ult i64 %7, %2
  br i1 %8, label %4, label %9, !llvm.loop !23

9:                                                ; preds = %4, %9
  %.1 = phi i64 [ %13, %9 ], [ 0, %4 ]
  %10 = add i64 %.1, 1
  %11 = getelementptr inbounds i32, ptr %0, i64 %10
  %12 = load i32, ptr %11, align 4
  %13 = add i64 %.1, 1
  %14 = icmp ult i64 %13, %2
  br i1 %14, label %9, label %15, !llvm.loop !24

15:                                               ; preds = %9
  ret void
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
!14 = distinct !{!14, !7}
!15 = distinct !{!15, !7}
!16 = distinct !{!16, !7}
!17 = distinct !{!17, !7}
!18 = distinct !{!18, !7}
!19 = distinct !{!19, !7}
!20 = distinct !{!20, !7}
!21 = distinct !{!21, !7}
!22 = distinct !{!22, !7}
!23 = distinct !{!23, !7}
!24 = distinct !{!24, !7}
