; ModuleID = 'main.ll'
source_filename = "main.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux-gnu"

@.str = private unnamed_addr constant [11 x i8] c"result=%d\0A\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main(i32 noundef %0, ptr noundef %1) #0 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca ptr, align 8
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  store i32 0, ptr %3, align 4
  store i32 %0, ptr %4, align 4
  store ptr %1, ptr %5, align 8
  call void @init_branch_stats()
  %8 = load i32, ptr %4, align 4
  %9 = icmp sgt i32 %8, 1
  call void @increment_cond_branch()
  br i1 %9, label %10, label %15

10:                                               ; preds = %2
  %11 = load ptr, ptr %5, align 8
  %12 = getelementptr inbounds ptr, ptr %11, i64 1
  %13 = load ptr, ptr %12, align 8
  call void @increment_direct_call()
  %14 = call i32 @atoi(ptr noundef %13) #4
  call void @increment_uncond_branch()
  br label %16

15:                                               ; preds = %2
  call void @increment_uncond_branch()
  br label %16

16:                                               ; preds = %15, %10
  %17 = phi i32 [ %14, %10 ], [ 8, %15 ]
  store i32 %17, ptr %6, align 4
  %18 = load i32, ptr %6, align 4
  call void @increment_direct_call()
  %19 = call i32 @foo(i32 noundef %18)
  store i32 %19, ptr %7, align 4
  %20 = load i32, ptr %7, align 4
  call void @increment_direct_call()
  %21 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %20)
  call void @print_branch_stats()
  call void @increment_return()
  ret i32 0
}

declare void @init_branch_stats() #1

; Function Attrs: nounwind willreturn memory(read)
declare i32 @atoi(ptr noundef) #2

; Function Attrs: noinline nounwind uwtable
define internal i32 @foo(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  store i32 0, ptr %4, align 4
  store i32 0, ptr %5, align 4
  call void @increment_uncond_branch()
  br label %8

8:                                                ; preds = %25, %1
  call void @increment_loop_header()
  %9 = load i32, ptr %5, align 4
  %10 = load i32, ptr %3, align 4
  %11 = icmp slt i32 %9, %10
  call void @increment_cond_branch()
  br i1 %11, label %12, label %28

12:                                               ; preds = %8
  %13 = load i32, ptr %5, align 4
  %14 = icmp eq i32 %13, 2
  call void @increment_cond_branch()
  br i1 %14, label %15, label %16

15:                                               ; preds = %12
  call void @increment_uncond_branch()
  br label %25

16:                                               ; preds = %12
  %17 = load i32, ptr %5, align 4
  %18 = icmp eq i32 %17, 5
  call void @increment_cond_branch()
  br i1 %18, label %19, label %20

19:                                               ; preds = %16
  call void @increment_uncond_branch()
  br label %29

20:                                               ; preds = %16
  %21 = load i32, ptr %5, align 4
  call void @increment_direct_call()
  %22 = call i32 @helper(i32 noundef %21)
  %23 = load i32, ptr %4, align 4
  %24 = add nsw i32 %23, %22
  store i32 %24, ptr %4, align 4
  call void @increment_uncond_branch()
  br label %25

25:                                               ; preds = %20, %15
  %26 = load i32, ptr %5, align 4
  %27 = add nsw i32 %26, 1
  store i32 %27, ptr %5, align 4
  call void @increment_uncond_branch()
  br label %8, !llvm.loop !6

28:                                               ; preds = %8
  call void @increment_uncond_branch()
  br label %29

29:                                               ; preds = %28, %19
  %30 = load i32, ptr %3, align 4
  store i32 %30, ptr %6, align 4
  call void @increment_uncond_branch()
  br label %31

31:                                               ; preds = %47, %29
  call void @increment_loop_header()
  %32 = load i32, ptr %6, align 4
  %33 = icmp sgt i32 %32, 0
  call void @increment_cond_branch()
  br i1 %33, label %34, label %48

34:                                               ; preds = %31
  %35 = load i32, ptr %6, align 4
  %36 = srem i32 %35, 2
  %37 = icmp ne i32 %36, 0
  %38 = zext i1 %37 to i64
  %39 = select i1 %37, i32 1, i32 2
  %40 = load i32, ptr %4, align 4
  %41 = add nsw i32 %40, %39
  store i32 %41, ptr %4, align 4
  %42 = load i32, ptr %6, align 4
  %43 = add nsw i32 %42, -1
  store i32 %43, ptr %6, align 4
  %44 = load i32, ptr %6, align 4
  %45 = icmp eq i32 %44, 3
  call void @increment_cond_branch()
  br i1 %45, label %46, label %47

46:                                               ; preds = %34
  call void @increment_uncond_branch()
  br label %49

47:                                               ; preds = %34
  call void @increment_uncond_branch()
  br label %31, !llvm.loop !8

48:                                               ; preds = %31
  call void @increment_uncond_branch()
  br label %49

49:                                               ; preds = %48, %46
  store i32 0, ptr %7, align 4
  call void @increment_uncond_branch()
  br label %50

50:                                               ; preds = %61, %49
  call void @increment_loop_header()
  %51 = load i32, ptr %7, align 4
  %52 = load i32, ptr %4, align 4
  %53 = add nsw i32 %52, %51
  store i32 %53, ptr %4, align 4
  %54 = load i32, ptr %7, align 4
  %55 = add nsw i32 %54, 1
  store i32 %55, ptr %7, align 4
  %56 = load i32, ptr %7, align 4
  %57 = icmp eq i32 %56, 2
  call void @increment_cond_branch()
  br i1 %57, label %58, label %60

58:                                               ; preds = %50
  %59 = load i32, ptr %4, align 4
  store i32 %59, ptr %2, align 4
  call void @increment_uncond_branch()
  br label %70

60:                                               ; preds = %50
  call void @increment_uncond_branch()
  br label %61

61:                                               ; preds = %60
  %62 = load i32, ptr %7, align 4
  %63 = icmp slt i32 %62, 4
  call void @increment_cond_branch()
  br i1 %63, label %50, label %64, !llvm.loop !9

64:                                               ; preds = %61
  %65 = load i32, ptr %4, align 4
  %66 = icmp slt i32 %65, 0
  call void @increment_cond_branch()
  br i1 %66, label %67, label %68

67:                                               ; preds = %64
  call void @increment_direct_call()
  call void @exit(i32 noundef 2) #5
  unreachable

68:                                               ; preds = %64
  %69 = load i32, ptr %4, align 4
  store i32 %69, ptr %2, align 4
  call void @increment_uncond_branch()
  br label %70

70:                                               ; preds = %68, %58
  %71 = load i32, ptr %2, align 4
  call void @increment_return()
  ret i32 %71
}

declare i32 @printf(ptr noundef, ...) #1

declare void @print_branch_stats() #1

; Function Attrs: noinline nounwind uwtable
define internal i32 @helper(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  %4 = load i32, ptr %3, align 4
  %5 = and i32 %4, 1
  %6 = icmp ne i32 %5, 0
  call void @increment_cond_branch()
  br i1 %6, label %7, label %10

7:                                                ; preds = %1
  %8 = load i32, ptr %3, align 4
  %9 = mul nsw i32 %8, 3
  store i32 %9, ptr %2, align 4
  call void @increment_uncond_branch()
  br label %13

10:                                               ; preds = %1
  %11 = load i32, ptr %3, align 4
  %12 = add nsw i32 %11, 10
  store i32 %12, ptr %2, align 4
  call void @increment_uncond_branch()
  br label %13

13:                                               ; preds = %10, %7
  %14 = load i32, ptr %2, align 4
  call void @increment_return()
  ret i32 %14
}

; Function Attrs: noreturn nounwind
declare void @exit(i32 noundef) #3

declare void @increment_cond_branch()

declare void @increment_direct_call()

declare void @increment_uncond_branch()

declare void @increment_return()

declare void @increment_loop_header()

attributes #0 = { noinline nounwind uwtable "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic" "target-features"="+fp-armv8,+neon,+outline-atomics,+v8a,-fmv" }
attributes #1 = { "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic" "target-features"="+fp-armv8,+neon,+outline-atomics,+v8a,-fmv" }
attributes #2 = { nounwind willreturn memory(read) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic" "target-features"="+fp-armv8,+neon,+outline-atomics,+v8a,-fmv" }
attributes #3 = { noreturn nounwind "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic" "target-features"="+fp-armv8,+neon,+outline-atomics,+v8a,-fmv" }
attributes #4 = { nounwind willreturn memory(read) }
attributes #5 = { noreturn nounwind }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 1}
!5 = !{!"Ubuntu clang version 18.1.3 (1ubuntu1)"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
!9 = distinct !{!9, !7}
