First time contributing
=======================

Compile Cutter
--------------
The first step before starting to add code to Cutter, is to build it on your environment. Whether it is Linux, Windows, or macOS, we have you covered and detailed the instructions to compile Cutter on our :doc:`../building`. Once you are done and Cutter was built successfully, test that it works correctly and continue to the next steps.


Read documentation
------------------

Read :doc:`../developers-docs`.

Get familiar with current situation
-----------------------------------

Run Cutter and get familiar with how the functionality you want to modify works currently. For example, if it is a dialog, make sure you understand its role and how our users are using it. If it is a bug, try to repeat it using the instructions from bug report. If you are inexperienced with binary analysis tasks, you can always ask other developers in the team about the feature and how it should be used. The team and community will be happy to help and instruct. 

Find the source files implementing the feature you want to work on. By doing this, practice searching code in your editor, this is an important skill. We suggest searching for files with the name of the feature. For example, if you are interested in improving the "Sections" widget, you can find the source code of this feature in `SectionsWidget.cpp` and `SectionsWidget.h`. Alternatively, you can try and look for specific unique strings that exist in the dialog, widget or feature you want to improve. Usually, when searching the entire code base for these strings, you'll land on a file related to this feature, whether it is a `.cpp` file or a `.ui` file. From there it will be easy to navigate your way to the right place.


Work on your feature or bug-fix 
-------------------------------

Come up with a plan for things that need to be done and discuss it in GitHub issue. Just because something was mentioned in issue description doesn't necessarily means that it is still relevant or good feature.
If you don't know to implement something check if any of the existing code implements similar similar behavior in the same widget or similar widgets. If you do copy an existing code consider why it did things the way it did, the same factors might not apply in your case or the old code is simply bad.

Open a Pull request
-------------------
When you are done, and the code changes, additions, and modifications are in place and comitted, it is time to move forward and opening a new Pull Request. Please remember to follow the PR template upon opening a new one.

In the Pull Request template you will be required to add a "Test plan". For example, if you performed GUI changes, demonstrate it by posting a screenshot.
Make a basic list of steps for checking that changes are working as expected. This is also a good point to consider any potential edge-cases or different kinds of interesting inputs if you didn't already do it while writing the code. Perform the steps you described when making the PR even if they seem trivial and you did them during development, it helps to catch any mistakes done during the final cleanup and making sure you didn't forget anything.