; ModuleID = 'test/dev/test.c'
source_filename = "test/dev/test.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@0 = private unnamed_addr constant [12 x i8] c"Hello World\00", align 1

define i32 @main() {
  %1 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([12 x i8], [12 x i8]* @0, i32 0, i32 0))
  ret i32 0
}

declare i32 @printf(i8*, ...)
