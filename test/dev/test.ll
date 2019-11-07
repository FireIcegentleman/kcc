; ModuleID = 'test/dev/test.c'
source_filename = "test/dev/test.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@0 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

define i32 @main() {
  %a = alloca i32, align 4
  store i32 10, i32* %a, align 4
  %1 = load i32, i32* %a, align 4
  %2 = icmp ne i32 %1, 0
  %3 = select i1 %2, i32 4, i32 8
  %4 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @0, i32 0, i32 0), i32 %3)
  ret i32 0
}

declare i32 @printf(i8*, ...)
