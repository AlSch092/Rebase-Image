# Rebase-Image
Research into how a program's flow can be obfuscated using image rebasing  

The goal behind this snippet/concept is to create a copy of a program image and then change execution context such that we are running inside that copy instead of the original. What this does is trick any attackers into breakpointing on routines inside the original image, which will of course never be called! After we've jumped to our new context, the original image stays in memory but has no activity or threads running in it.  

Step-by-step:  
1) Get size of image + original address, use `VirtualAlloc()` to get some memory with the same size as the image.
2) Modify PEB OptionalHeader member `ImageBase`, updating it to the address we got from `VirtualAlloc()` in the above step
3) Copy all bytes from original image into our memory space from the above step using `memcpy`.  
4) Grab the offset of our `main()` function by subtracting the `main()` routine's address from the original image base address.  
5) Calculate new address for `main()` function in re-based context by adding the offset in the above step to the memory space address in the first step
6) Create new thread at the re-based `main()` address, execution resumes at `main()` in our new image space
7) End current thread (or, optionally use `WaitForSingleObject()` to make the original main thread wait for program termination/other thread to end)
8) Use `TerminateProcess()` when you're done with your program, as exiting threads naturally after these modifications can lead to program hangs (ZwTerminateThread will loop forever)

Possible benefits:  
1) Tricks manual debugging/analysis at runtime by changing execution context to an 'unnamed module' - breakpoints on original image routines won't be hit 
2) Can be used in combination with tricks such as image re-mapping and function pointers set at runtime
3) Execution can 'randomly' jump between routines in the original image and the copy (if you get fancy and write an algo for this), leading to high levels of initial confusion.  
  
All steps are demonstrated in the file rebase.c for your convenience, I do hope you enjoyed this topic and learned something!  
