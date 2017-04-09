## Code formatting

We use [AStyle 2.06](https://sourceforge.net/projects/astyle/files/astyle/astyle%202.06/) to format the code. The command line for formatting the code according to the style is:

```bash
AStyle --style=allman --convert-tabs --align-pointer=name --align-reference=name --indent=spaces --indent-namespaces --indent-col1-comments --pad-oper --pad-header --unpad-paren --keep-one-line-blocks --close-templates $(git ls-files *.cpp *.h *.c *.hpp)
```

**If in doubt, check the source around you and follow that style!**
