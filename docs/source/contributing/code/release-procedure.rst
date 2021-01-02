Release Procedure
=================

1. Update translations submodule `<https://github.com/rizinorg/cutter-translations>`_
    1. The latest archive from Crowdin should already be in the repository, if not make sure to merge any automated Pull Request from Crowdin (e.g. https://github.com/rizinorg/cutter-translations/pull/9)
    2. Update submodule in cutter
2. If there is a desire to keep working in the master branch, create branch for the release and do all the following work there.
3. Lock rzghidra and rzdec versions downloaded by packaging scripts. Specify a tag or commit hash.
4. Update version
    #. appveyor.yml
    #. docs/sourc/conf.py
    #. docs/source/index.rst
    #. CMakeLists.txt
    #. Cutter.appdata.xml
    #. To be safe, search the code base for the previous version number.
5. Create a tag for the release candidate. For example, for the `v1.11.0` release you'd do something like this:
    #. ``git tag v1.11.0-rc1``
    #. ``git tag push origin v1.11.0-rc1``
6. Create a GitHub release, mark it as pre-release save draft, set the tag to v1.11.0-rc1
7. Wait for packages to build
8. On all operating systems do the `Basic testing procedure`_ to ensure nothing badly broken.
9. If any major problems are found, open an issue and fix them. If a release branch is used fix them in master and cherry pick into release branch. If the amount of changes is sufficiently large repeat from step 3. increasing rc number by one.
10. Update version to 1.11.0
11. Create tag
12. Create release
    * Fill the release notes in the Release description. Preparing release notes can begin earlier. Compare current master or release branch against previous release to find all the changes. Choose the most important ones. Don't duplicate the commit log. Release notes should be a summary for people who don't want to read whole commit log. Group related changes together under titles such as "New features", "Bug Fixes", "Decompiler", "Rizin" and similar.
13. Prepare announcement tweets and messages to send in the Telegram group, reddit, and others.
14. Close milestone if there was one



Bugfix Release
--------------
The process for bugfix release is similar no normal release procedure described above with some differences.

* Create the branch from the last tag instead of master or reuse the branch from x.y.0 release if it was already created.
* Cherry pick required bugfixes from master into the branch.
* Increase the third version number x.y.n into x.y.(n+1) .


Basic testing procedure
-----------------------

This isn't intended as exhaustive testing process, just some simple steps to make sure nothing is badly broken.
If it makes sense repeat the step multiple times at different offsets and click around increase the chance of noticing common problems that doesn't happen 100% of time.

* Open a simple executable like ``/bin/ls`` or ``calc.exe``
* Make sure that the upgraded layout isn't completely broken
* The Disassembly widget shows proper disassembly.
* Bundled plugins work
   * Open decompiler and select ghidra, it shows some C code at least for some functions
   * Open rzdec in decompiler widget, make sure it shows code
* Test that sample python plugin works
* Try debugger
   * Insert breakpoint in main
   * Start debugging
   * Go to main using function widget, make sure relocation was done correctly and you see code instead of unmapped memory and breakpoint is where you placed
   * Click continue until you hit breakpoint in main
* Delete cutter settings file, and test that clean start works and layout isn't broken
