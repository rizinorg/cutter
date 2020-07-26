Patching
==============================

Write String
----------------------------------------
**Description:** Write ASCII string at the current location. If multiple bytes are selected, the string will be written from the start of the selection. Please note, this item won't write a null-terminated nor wide string. Please see the other items for these features.

**Steps:** Right-click on a byte and select ``Edit -> Write string``  

Write Length and String
----------------------------------------
**Description:** Write a length prefix followed by ASCII string at the current location. If multiple bytes are selected, the string will be written from the start of the selection. Please note, although similar to BSTR in its concept, this item would not add a null terminator WCHAR at the end of the string.

**Steps:** Right-click on a byte and select ``Edit -> Write length and string``  


Write Wide String
----------------------------------------
**Description:** Write null-terminated wide string at the current location. If multiple bytes are selected, the string will be written from the start of the selection.

**Steps:** Right-click on a byte and select ``Edit -> Write wide string``  


Write Null-Terminated String
----------------------------------------
**Description:** Write a null-terminated ASCII string at the current location. If multiple bytes are selected, the string will be written from the start of the selection.

**Steps:** Right-click on a byte and select ``Edit -> Write zero terminated string``  


Write Encoded\\Decoded Base64 String
----------------------------------------
**Description:** Write an encoded or decoded base64 string at the current location. If multiple bytes are selected, the string will be written from the start of the selection.

**Steps:** Right-click on a byte and select ``Edit -> Write De\Encoded Base64 string`` . On the dialog that will open choose whether you want to encode a string or decode one.


Write Zeroes
----------------------------------------
**Description:** Write null-bytes at the current location. The number of null-bytes is specified by the user. If multiple bytes are selected, the null-bytes will be written from the start of the selection.

**Steps:** Right-click on a byte and select ``Edit -> Write zeroes``. On the dialog that will open, specify how many null-bytes you'd like to write.


Write Random Bytes
----------------------------------------
**Description:** Write random bytes at the current location. The number of bytes is specified by the user. If multiple bytes are selected, the null-bytes will be written from the start of the selection.

**Steps:** Right-click on a byte and select ``Edit -> Write random bytes``. On the dialog that will open, specify how many bytes you'd like to write.


Duplicate Bytes From Offset
----------------------------------------
**Description:** Duplicate N bytes from an offset to the current location. The number of bytes to duplicated and the offset of origin are specified by the user. If multiple bytes are selected, the bytes will be written from the start of the selection. A preview pane will display the bytes to be copied.

**Steps:** Right-click on a byte and select ``Edit -> Duplicate from offset``. On the dialog that will open, specify the offset from which to copy, and how many bytes to copy.  


Increment/Decrement Bytes
----------------------------------------
**Description:** Increment or decrement Byte, Word, Dword or Qword at the current location. The value to add or subtract from the location is specified by the user. If multiple bytes are selected, the function will apply on the start of the selection.

**Steps:** Right-click on a byte and select ``Edit -> Increment/Decrement``. On the dialog that will open, specify if you'd like to modify a Byte, Word, Dword or Qword, choose the value of the operation, and choose whether you want to increment or decrement.