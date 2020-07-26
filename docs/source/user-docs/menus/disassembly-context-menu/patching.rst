Patching
==============================

Edit Instruction
----------------------------------------
**Description:** Edit the current instruction by typing a sequence of one or more instructions. Cutter will automatically fetch a preview of the bytes that are constructing the instruction.  

**Steps:** Right-click on an instruction and choose ``Edit -> Instruction``  

Edit Bytes
----------------------------------------
**Description:** Edit the bytes of the current instruction by typing a sequence of bytes. Cutter will automatically disassemble a preview of the instructions that are create by the typed bytes.  

**Steps:** Right-click on an instruction and choose ``Edit -> Bytes``  

NOP Instruction
----------------------------------------
**Description:** Fill the content of the instruction with NOP instructions. Cutter will fill the instructions with NOP as the length of the bytes constructing the instruction.   

**Steps:** Right-click on an instruction and choose ``Edit -> Nop Instruction``  

Reverse Jump
----------------------------------------
**Description:** On conditional jumps, Cutter will detect the inverted conditional instruction and will replace it. For example, from ``je`` to ``jne``.  

**Steps:** Right-click on an instruction and choose ``Edit -> Reverse Jump``