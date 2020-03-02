Set as Code\Data\String
==============================

Set as Code
----------------------------------------
**Description:** Set the current instruction to Code. This will force Cutter to display the current instruction as Code.  

**Steps:** Set as... -> Code  

**Shortcut:** :kbd:`C`    

Set as String (auto-detect length) 
----------------------------------------
**Description:** Set the current location to String. This will tell Cutter to treat the current address as a string and will auto-detect the length (e.g by looking for a string null-terminator).   

**Steps:** Right click on an instruction and choose ``Set as... -> String... -> Auto-detect``  

**Shortcut:** :kbd:`A`  

Remove string definition
----------------------------------------
**Description:** Remove a defined string in this address. Cutter will then treat this location as a code.  

**Steps:** Right click on an instruction and choose ``Set as... -> String... -> Remove``.  

Set as String (Advance dialog)
----------------------------------------
**Description:** Set the current location to String. This will open a String definition dialog in which the user will supply the length and the type (ASCII, UTF8, ...) of the string. Cutter will then treat the current address as the defined string.  

**Steps:** Right click on an instruction and choose ``Set as... -> String... -> Advanced``.  

Set as data (bytes)
----------------------------------------
**Description:** Convert the instruction to data of Bytes.   

**Steps:** Right click on an instruction and choose ``Set as... -> Data... -> Byte``  

Set as data (Word)
----------------------------------------
**Description:** Convert the instruction to data of Words.  

**Steps:** Right click on an instruction and choose ``Set as... -> Data... -> Word``  

Set as data (Dword)
----------------------------------------
**Description:** Convert the instruction to data of Dwords.  

**Steps:** Right click on an instruction and choose ``Set as... -> Data... -> Dword``  

Set as data (Qword)
----------------------------------------
**Description:** Convert the instruction to data of Qwords.  

**Steps:** Right click on an instruction and choose ``Set as... -> Data... -> Qword``  

Set as data (Advanced)
----------------------------------------
**Description:** Open an advanced dialog to define the custom data type of the current instruction.  

**Steps:** Right click on an instruction and choose ``Set as... -> Data... -> ...``  

**Shortcut:** :kbd:`*`