Debug Menu
==============================


Start Debugging
----------------------------------------
**Description:** Start the debugging session of the current loaded binary.  

**Steps:** Debug -> Start debug  

**Shortcut:** :kbd:`F9`  

Start Emulation
----------------------------------------
**Description:** Start an emulation session on the current loaded binary. Cutter supports emulation of different file formats. Unlike debugging, emulation isn't really executing the binary, but only emulating the instructions. This is very strong feature for analysis of self-contained functions or programs, to analyze cryptographic algorithms or to deobfuscate data. Emulation isn't limited by the running platform, so Linux files such as ELF can be emulated on Windows platforms, and DLL can be emulated on Linux.  

**Steps:** Debug -> Start emulation  

Attach to process
----------------------------------------
**Description:** Attach Cutter's debugger to a running process, instead of spawning a new process.  

**Steps:** Debug -> Attach to process  

Connect to a Remote Debugger
----------------------------------------
**Description:** Connect Cutter to a remote debugger such as GDB ot WinDbg by providing IP and Port of the remote debugger..  

**Steps:** Debug -> Connect to a remote debugger  

Step Into
----------------------------------------
**Description:** Execute a single assembler instruction, stepping into functions and loops.  

**Steps:** Debug -> Step  

**Shortcut:** :kbd:`F7`  

Step Over
----------------------------------------
**Description:** Execute a single assembler instruction, stepping over functions and procedures. The functions will not be skipped and will be executed by Cutter. The execution will pause when reaching the instruction after the function call.    

**Steps:** Debug -> Step over  

**Shortcut:** :kbd:`F8`  

Step Out
----------------------------------------
**Description:** Execute the code and suspends execution when the current function returns  

**Steps:** Debug -> Step out  

**Shortcut:** :kbd:`Ctrl` + :kbd:`F8`  

Continue
----------------------------------------
**Description:** Continue the execution of the running program. The execution will stop when reached a breakpoint, when manually suspended by the user, or when the running program quits.   

**Steps:** Debug -> Continue  

**Shortcut:** :kbd:`F5`  

Continue Until Call
----------------------------------------
**Description:** Continue the execution of the program until a function call is reached.  

**Steps:** Debug -> Continue until call  

Continue Until Syscall
----------------------------------------
**Description:** Continue the execution of the program until a Syscall is reached.  

**Steps:** Debug -> Continue until syscall
