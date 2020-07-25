Debug Context Menu
==============================

Continue Until Line
----------------------------------------
**Description:** Continue the execution of the program until reached the selected offset. The program is not guaranteed to ever reach this address and will keep running until exited or until reached another breakpoint. If other breakpoints hit before reaching this line, they will be triggered and pause the execution. *This option is only available on Debug or Emulation modes*.      

**Steps:** While in Debug or Emulation modes, right-click and address and choose ``Debug -> Continue until line``.  

Set Program Counter (PC)
----------------------------------------
**Description:** Set the Program Counter of the debuggee to the current offset. For example, on an Intel 64bit program, Cutter will set the value of the RIP register to the current address.  *This option is only available on Debug or Emulation modes*.  

**Steps:** While in Debug or Emulation modes, right-click and address and choose ``Debug -> Set PC``.
